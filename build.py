import os
import shutil
import subprocess
from pathlib import Path


CONFIG = "build.conf"


def parse_config(filename):
    config = {}
    section = None

    with open(filename, encoding="utf-8") as file:
        for raw_line in file:
            line = raw_line.strip()

            if not line or line.startswith("#"):
                continue

            if line.startswith("[") and line.endswith("]"):
                section = line[1:-1]
                config[section] = {}
                continue

            if "=" not in line or section is None:
                continue

            key, value = line.split("=", 1)
            config[section][key.strip()] = value.strip()

    return config


def split_list(value):
    return [
        item.strip()
        for item in value.split(",")
        if item.strip()
    ]


def is_true(value):
    return value.lower() in {
        "true",
        "yes",
        "1",
        "on",
    }


def find_sources(root, recursive, extensions, excluded_dirs):
    if recursive:
        iterator = root.rglob("*")
    else:
        iterator = root.glob("*")

    files = []

    for path in iterator:
        if not path.is_file():
            continue

        if path.suffix not in extensions:
            continue

        if any(
            part in excluded_dirs
            for part in path.parts
        ):
            continue

        files.append(path)

    return sorted(files)


def clean_output(output):
    if output.exists():
        if output.is_dir():
            shutil.rmtree(output)
        else:
            output.unlink()

        print(f"Cleaned: {output}")


def main():
    config = parse_config(CONFIG)

    project = config.get("project", {})
    source = config.get("source", {})
    target = config.get("target", {})
    compiler = config.get("compiler", {})
    include = config.get("include", {})
    exclude = config.get("exclude", {})
    build = config.get("build", {})

    name = project.get("name", "app")

    root = Path(
        source.get("root", ".")
    )

    recursive = is_true(
        source.get("recursive", "true")
    )

    extensions = set(
        split_list(
            source.get(
                "extensions",
                ".cpp,.c,.h,.hpp"
            )
        )
    )

    excluded_dirs = set(
        split_list(
            exclude.get("dirs", "")
        )
    )

    output = Path(
        target.get(
            "output",
            f"build/{name}"
        )
    )

    # ------------------------------------------------------------
    # Clean
    # ------------------------------------------------------------

    if is_true(build.get("clean", "false")):
        clean_output(output)

    # ------------------------------------------------------------
    # Find source files
    # ------------------------------------------------------------

    all_files = find_sources(
        root,
        recursive,
        extensions,
        excluded_dirs
    )

    # Headers are discovered but are not compiled directly.
    source_files = [
        path
        for path in all_files
        if path.suffix in {
            ".cpp",
            ".c",
        }
    ]

    if not source_files:
        print("No source files found.")
        return 1

    # ------------------------------------------------------------
    # Create output directory
    # ------------------------------------------------------------

    output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    # ------------------------------------------------------------
    # Compiler
    # ------------------------------------------------------------

    command = [
        compiler.get("command", "g++"),
        f"-std={compiler.get('standard', 'c++20')}",
    ]

    # ------------------------------------------------------------
    # Debug
    # ------------------------------------------------------------

    if is_true(build.get("debug", "false")):
        command.append("-g")

    # ------------------------------------------------------------
    # Optimization
    # ------------------------------------------------------------

    optimization = build.get(
        "optimization",
        ""
    ).strip()

    if optimization:
        if optimization in {
            "0",
            "1",
            "2",
            "3",
            "s",
            "fast",
        }:
            command.append(
                f"-O{optimization}"
            )
        else:
            print(
                f"Invalid optimization level: "
                f"{optimization}"
            )
            return 1

    # ------------------------------------------------------------
    # Warnings
    # ------------------------------------------------------------

    if is_true(
        build.get("warnings", "false")
    ):
        command.extend([
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        ])

    # ------------------------------------------------------------
    # Custom compiler flags
    # ------------------------------------------------------------

    flags = compiler.get(
        "flags",
        ""
    ).strip()

    if flags:
        command.extend(
            flags.split()
        )

    # ------------------------------------------------------------
    # Include directories
    # ------------------------------------------------------------

    for path in split_list(
        include.get("paths", "")
    ):
        command.append(
            f"-I{path}"
        )

    # ------------------------------------------------------------
    # Source files
    # ------------------------------------------------------------

    command.extend(
        str(path)
        for path in source_files
    )

    # ------------------------------------------------------------
    # Output
    # ------------------------------------------------------------

    command.extend([
        "-o",
        str(output),
    ])

    # ------------------------------------------------------------
    # Display
    # ------------------------------------------------------------

    print(f"Project: {name}")
    print()

    print("Source files:")
    for path in source_files:
        print(f"  {path}")

    print()

    print("Build options:")
    print(
        f"  Debug:        "
        f"{is_true(build.get('debug', 'false'))}"
    )
    print(
        f"  Optimization: "
        f"O{optimization or 'none'}"
    )
    print(
        f"  Warnings:     "
        f"{is_true(build.get('warnings', 'false'))}"
    )
    print(
        f"  Clean:        "
        f"{is_true(build.get('clean', 'false'))}"
    )

    print()
    print("Command:")
    print(" ".join(command))
    print()

    # ------------------------------------------------------------
    # Build
    # ------------------------------------------------------------

    result = subprocess.run(command)

    if result.returncode != 0:
        print()
        print("Build failed.")
        return result.returncode

    print()
    print(f"Build successful: {output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())