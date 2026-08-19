import os

_config = {}

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

        if line.endswith("{"):
            name = line[:-1].strip()
            new_block = {}
            stack[-1][name] = new_block
            stack.append(new_block)
            continue

        if line == "}":
            if len(stack) == 1:
                raise ValueError(f"Unexpected '}}' on line {line_number}")
            stack.pop()
            continue

        if "=" in line:
            key, value = line.split("=", 1)
            key = key.strip()
            value = value.strip()

            if len(value) >= 2 and value[0] == value[-1] and value[0] in "\"'":
                value = value[1:-1]

            stack[-1][key] = value
            continue

        raise ValueError(f"Invalid syntax on line {line_number}")

    if len(stack) != 1:
        raise ValueError("Unclosed '{' block")

    _config = root


def get(var):
    # Nested config: get("Routes.Pages.about")
    value = _config

    for key in var.split("."):
        if isinstance(value, dict) and key in value:
            value = value[key]
        else:
            # Fall back to normal environment variable
            return os.environ.get(var)

    return value