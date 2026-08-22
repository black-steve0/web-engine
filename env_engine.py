import os

_config = {}

def parse_value(value):
    value = value.strip()

    # Quoted string
    if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
        return value[1:-1]

    # Boolean
    if value.lower() == "true":
        return True

    if value.lower() == "false":
        return False

    # {...} = list
    if value.startswith("{") and value.endswith("}"):
        content = value[1:-1].strip()

        if not content:
            return []

        return [
            item.strip().strip("\"'")
            for item in content.split(",")
        ]

    return value


def parse_route_value(value):
    """
    Parses:

        "about.html"{--root, default}

    into:

        {
            "file": "about.html",
            "options": ["--root", "default"]
        }
    """

    value = value.strip()

    # No options
    if not value.endswith("}"):
        return {
            "file": parse_value(value),
            "options": []
        }

    # Find the opening { belonging to the options
    options_start = value.rfind("{")

    if options_start == -1:
        return {
            "file": parse_value(value),
            "options": []
        }

    file_value = value[:options_start].strip()
    options_value = value[options_start:].strip()

    return {
        "file": parse_value(file_value),
        "options": parse_value(options_value)
    }


def load_env(filename=".env"):
    global _config

    with open(filename) as f:
        lines = f.readlines()

    root = {}
    stack = [root]

    for line_number, line in enumerate(lines, 1):
        line = line.strip()

        if not line or line.startswith("#") or line.startswith("//"):
            continue

        # Block
        if line.endswith("{"):
            name = line[:-1].strip()

            if not name:
                raise ValueError(
                    f"Invalid block on line {line_number}"
                )

            new_block = {}
            stack[-1][name] = new_block
            stack.append(new_block)
            continue

        # Close block
        if line == "}":
            if len(stack) == 1:
                raise ValueError(
                    f"Unexpected '}}' on line {line_number}"
                )

            stack.pop()
            continue

        # Key/value
        if "=" in line:
            key, value = line.split("=", 1)

            key = key.strip()
            value = value.strip()

            # Route syntax:
            # key="file.html"{options}
            if value.startswith('"') or value.startswith("'"):
                closing_quote = value.find(value[0], 1)

                if closing_quote != -1:
                    after_quote = value[closing_quote + 1:].strip()

                    if after_quote.startswith("{"):
                        stack[-1][key] = parse_route_value(value)
                        continue

            stack[-1][key] = parse_value(value)
            continue

        raise ValueError(f"Invalid syntax on line {line_number}")

    if len(stack) != 1:
        raise ValueError("Unclosed '{' block")

    _config = root


def get(var):
    value = _config

    for key in var.split("."):
        if isinstance(value, dict) and key in value:
            value = value[key]
        else:
            return os.environ.get(var)

    return value