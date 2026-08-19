import threading
import conf
import time
import sqlite3
from pathlib import Path


def home():
    return Path(conf.base) / conf.dirs["html"] / "index.html"

def icon():
    return Path(conf.base) / conf.dirs['img'] / "favicon.ico"

def not_found():
    return Path(conf.base) / conf.dirs['html'] / "defaults/404.html"

def get_db_connection():
    return sqlite3.connect('db/product.db')

def get_file(route, type="html"):

    def handler():
        r = conf.dynamic_routes[type].get(route)

        if not r: return not_found()
        return Path(conf.base) / conf.dirs[type] / (r + ".html" if type == "html" else route)
    
    return handler

builtin_routes = {
    "/": home,
    "/favicon.ico": icon,
}

def init():
    conf.update()
    threading.Thread(target=updater, daemon=True).start()

def get(url):
    builtin = builtin_routes.get(url)

    if builtin:
        return builtin
    
    for key in conf.dirs:
        if url[1:].startswith(key):
            return get_file(url[2+len(key)::], key)

    return get_file(url[1::])

def updater():
    while True:
        try:
            conf.update()

            time.sleep(10)
        except(Exception):
            time.sleep(1)
        
