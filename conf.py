import env_engine

dirs = {}
dynamic_routes = {}
base = ""


def update():
    global dirs, dynamic_routes, base

    env_engine.load_env("routes.conf")

    dirs = env_engine.get("dirs")
    dynamic_routes = env_engine.get("routes")
    base = env_engine.get("base")