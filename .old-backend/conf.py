import env_engine

def update():
    global dirs, dynamic_routes, base, MIME_TYPES, defaults, root

    env_engine.load_env("routes.conf")

    dirs = env_engine.get("dirs")
    dynamic_routes = env_engine.get("routes")
    base = env_engine.get("base")
    MIME_TYPES = env_engine.get("MIME")
    defaults = env_engine.get("defaults")
    root = env_engine.get("force-root")
    
