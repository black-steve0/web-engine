from pathlib import Path
import conf

def render_html(file_path, options):
    content = file_path.read_text(encoding="utf-8")

    print(bool(conf.root))

    if (bool(conf.root)):
        options.insert(0, "root")

    for option in options:
        prepend = option.startswith("--")
        name = option.lstrip("-")

        name = conf.defaults[name]

        template_path = (
            Path(conf.base)
            / conf.dirs["html"]
            / f"{name}"
        )

        template = template_path.read_text(encoding="utf-8")

        if prepend:
            content = template + content
        else:
            content = content + template

    return content