#!/usr/bin/env python3
"""Fast incremental C/C++ build driven by build.conf.

Rebuild decisions are based on CONTENT hashes, not timestamps:
  * Each source's "unit hash" = hash of the .cpp/.c plus every header it
    includes (taken from the compiler's -MMD depfile).
  * Only units whose unit hash changed are recompiled; `touch`, git checkouts
    and saves-without-edits cause no rebuilds.
  * File hashes are cached by (mtime, size), so unchanged files are never re-read.
  * A settings hash (compiler, flags, includes, linker) invalidates everything
    when the config changes, and does nothing when it doesn't.
  * Compiles run in parallel; ccache and mold/lld are used automatically if present.
  * One final binary (target.output). Object files are a hidden cache next to it.
"""

import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

CONFIG = "build.conf"
TRUE = {"true", "yes", "1", "on"}


def parse_config(filename):
    config, section = {}, None
    with open(filename, encoding="utf-8") as f:
        for raw in f:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            if line.startswith("[") and line.endswith("]"):
                section = line[1:-1]
                config[section] = {}
                continue
            if "=" not in line or section is None:
                continue
            k, v = line.split("=", 1)
            config[section][k.strip()] = v.strip()
    return config


def split_list(value):
    return [i.strip() for i in value.split(",") if i.strip()]


def is_true(value):
    return value.lower() in TRUE


def find_sources(root, recursive, extensions, excluded_dirs):
    files = []
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in excluded_dirs]
        for fn in filenames:
            if os.path.splitext(fn)[1] in extensions:
                files.append(Path(dirpath) / fn)
        if not recursive:
            break
    return sorted(files)


class Hasher:
    """Content hashes with a (mtime,size) fast path persisted between runs."""

    def __init__(self, known):
        self.known = known      # path -> [mtime_ns, size, hash] from last run
        self.fresh = {}         # same shape, for this run
        self.run_cache = {}

    def file(self, path):
        path = str(path)
        if path in self.run_cache:
            return self.run_cache[path]
        try:
            st = os.stat(path)
        except OSError:
            self.run_cache[path] = None
            return None
        prev = self.known.get(path)
        if prev and prev[0] == st.st_mtime_ns and prev[1] == st.st_size:
            h = prev[2]
        else:
            with open(path, "rb") as f:
                h = hashlib.blake2b(f.read(), digest_size=16).hexdigest()
        self.fresh[path] = [st.st_mtime_ns, st.st_size, h]
        self.run_cache[path] = h
        return h

    def unit(self, src, deps):
        """Combined hash of a source and all its headers; None if any is missing."""
        parts = []
        for p in sorted({str(src), *map(str, deps)}):
            h = self.file(p)
            if h is None:
                return None
            parts.append(f"{p}\0{h}")
        return hashlib.blake2b("\n".join(parts).encode(), digest_size=16).hexdigest()


def read_deps(depfile):
    try:
        text = Path(depfile).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return None
    text = text.replace("\\\n", " ")
    # Only the first rule matters; later lines are -MP phony header targets ("x.h:").
    _, _, rest = text.partition(": ")
    rest = rest.split("\n", 1)[0]
    return [p.replace("\\ ", " ") for p in re.split(r"(?<!\\)\s+", rest) if p]


def run_compile(cmd):
    r = subprocess.run(cmd, capture_output=True, text=True)
    return r.returncode, r.stdout + r.stderr


def load_state(path):
    try:
        s = json.loads(path.read_text())
        return {"settings": s.get("settings"), "linked": bool(s.get("linked")),
                "units": s.get("units", {}), "files": s.get("files", {})}
    except (OSError, ValueError):
        return {"settings": None, "linked": False, "units": {}, "files": {}}


def save_state(path, state):
    tmp = path.with_suffix(".tmp")
    tmp.write_text(json.dumps(state))
    os.replace(tmp, path)


def main():
    t0 = time.perf_counter()
    cfg = parse_config(CONFIG)
    project, source = cfg.get("project", {}), cfg.get("source", {})
    target, compiler = cfg.get("target", {}), cfg.get("compiler", {})
    include, exclude = cfg.get("include", {}), cfg.get("exclude", {})
    build, linker = cfg.get("build", {}), cfg.get("linker", {})

    name = project.get("name", "app")
    root = Path(source.get("root", "."))
    recursive = is_true(source.get("recursive", "true"))
    extensions = set(split_list(source.get("extensions", ".cpp,.c,.h,.hpp")))
    excluded = set(split_list(exclude.get("dirs", "")))
    output = Path(target.get("output", f"build/{name}"))
    objdir = output.parent / f".obj-{output.name}"
    state_file = objdir / "state.json"

    # --- Clean ---
    if is_true(build.get("clean", "false")):
        for p in (output, objdir):
            if p.is_dir():
                shutil.rmtree(p)
                print(f"Cleaned: {p}")
            elif p.exists():
                p.unlink()
                print(f"Cleaned: {p}")

    # --- Sources (headers are tracked via depfiles, never compiled) ---
    sources = [p for p in find_sources(root, recursive, extensions, excluded)
               if p.suffix in {".cpp", ".c"}]
    if not sources:
        print("No source files found.")
        return 1

    # --- Flags ---
    cc = compiler.get("command", "g++")
    base = [cc, f"-std={compiler.get('standard', 'c++20')}"]
    if is_true(build.get("debug", "false")):
        base.append("-g")

    opt = build.get("optimization", "").strip()
    if opt:
        if opt not in {"0", "1", "2", "3", "s", "fast"}:
            print(f"Invalid optimization level: {opt}")
            return 1
        base.append(f"-O{opt}")

    if is_true(build.get("warnings", "false")):
        base += ["-Wall", "-Wextra", "-Wpedantic"]
    base += compiler.get("flags", "").split()
    base += [f"-I{p}" for p in split_list(include.get("paths", ""))]

    launcher = []
    if is_true(build.get("ccache", "true")) and shutil.which("ccache"):
        launcher = ["ccache"]

    link_flags = linker.get("flags", "").split()
    chosen_ld = linker.get("linker", "auto")
    if chosen_ld == "auto":
        chosen_ld = next((l for l in ("mold", "lld")
                          if shutil.which(l) or shutil.which(f"ld.{l}")), "")
    if chosen_ld and chosen_ld != "default":
        link_flags.append(f"-fuse-ld={chosen_ld}")

    jobs = int(build.get("jobs", 0) or 0) or os.cpu_count() or 4

    # --- Settings hash ---
    settings_hash = hashlib.sha256(
        json.dumps([base, link_flags, launcher], sort_keys=True).encode()
    ).hexdigest()

    objdir.mkdir(parents=True, exist_ok=True)
    state = load_state(state_file)
    settings_changed = state["settings"] != settings_hash
    units = {} if settings_changed else dict(state["units"])
    hasher = Hasher(state["files"])

    # --- Decide what is stale (by content hash) ---
    stale, objects, wanted = [], [], set()
    for src in sources:
        tag = hashlib.sha1(str(src.resolve()).encode()).hexdigest()[:8]
        obj = objdir / f"{src.stem}.{tag}.o"
        dep = obj.with_suffix(".d")
        objects.append(obj)
        wanted.update((obj.name, dep.name))
        key = str(src)

        rebuild = True
        if key in units and obj.exists():
            deps = read_deps(dep)
            if deps is not None:
                h = hasher.unit(src, deps)
                rebuild = h is None or h != units[key]
        if rebuild:
            units.pop(key, None)
            cmd = launcher + base + ["-MMD", "-MP", "-MF", str(dep),
                                     "-c", str(src), "-o", str(obj)]
            stale.append((src, obj, dep, cmd))

    # Drop objects of deleted sources, and state for them
    for f in objdir.iterdir():
        if f.suffix in {".o", ".d"} and f.name not in wanted:
            f.unlink()
    keys = {str(s) for s in sources}
    units = {k: v for k, v in units.items() if k in keys}

    # --- Compile in parallel ---
    failed = False
    if stale:
        print(f"Compiling {len(stale)}/{len(sources)} file(s) on {min(jobs, len(stale))} thread(s)")
        with ThreadPoolExecutor(max_workers=min(jobs, len(stale))) as pool:
            futures = {pool.submit(run_compile, item[3]): item for item in stale}
            for fut in as_completed(futures):
                src, obj, dep, _ = futures[fut]
                code, out = fut.result()
                if code != 0:
                    failed = True
                    print(f"FAILED: {src}")
                else:
                    deps = read_deps(dep) or []
                    h = hasher.unit(src, deps)
                    if h is not None:
                        units[str(src)] = h
                if out.strip():
                    print(out, end="" if out.endswith("\n") else "\n")
    else:
        print("Everything up to date (no compiles needed).")

    need_link = bool(stale) or not output.exists() or settings_changed or not state["linked"]

    # Save progress now so successful objects survive a failed build
    state = {"settings": settings_hash, "linked": False if (stale or need_link) else True,
             "units": units, "files": hasher.fresh}
    save_state(state_file, state)

    if failed:
        print("Build failed.")
        return 1

    # --- Link only if needed ---
    if need_link:
        output.parent.mkdir(parents=True, exist_ok=True)
        r = subprocess.run([cc] + [str(o) for o in objects] + link_flags + ["-o", str(output)])
        if r.returncode != 0:
            print("Link failed.")
            return r.returncode
        state["linked"] = True
        save_state(state_file, state)
        print(f"Build successful: {output}  ({time.perf_counter() - t0:.2f}s)")
    else:
        print(f"Up to date: {output}  ({time.perf_counter() - t0:.2f}s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())