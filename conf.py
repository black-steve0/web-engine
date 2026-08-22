import env_engine

dirs = {}
dynamic_routes = {}
base = ""
MIME_TYPES = {}


def update():
    global dirs, dynamic_routes, base, MIME_TYPES

    env_engine.load_env("routes.conf")

    dirs = env_engine.get("dirs")
    dynamic_routes = env_engine.get("routes")
    base = env_engine.get("base")
    MIME_TYPES = env_engine.get("MIME")