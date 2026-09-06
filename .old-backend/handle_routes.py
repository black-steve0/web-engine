import threading
import conf
import time
import sqlite3
from pathlib import Path

def not_found():
    if (isinstance(conf.dynamic_routes["html"]["/:"], dict)):
        return (
            Path(conf.base)
            / conf.dirs["html"]
            / conf.dynamic_routes["html"]["/:"]["file"],
            conf.dynamic_routes["html"]["/:"]["options"]
        )
    return (
        Path(conf.base)
        / conf.dirs["html"]
        / conf.dynamic_routes["html"]["/:"],
        []
    )

# def not_found():
    # return Path(conf.base) / conf.dirs['html'] / conf.dynamic_routes['html']['/:']

def get_db_connection():
    return sqlite3.connect('db/product.db')

def get_file(route, type):

    def handler():
        r = conf.dynamic_routes[type].get(route)

        if not r:
            return not_found()

        if isinstance(r, dict):
            file = r["file"]
            options = r["options"]
        else:
            file = r
            options = []

        return Path(conf.base) / conf.dirs[type] / file, options

    return handler

def init():
    conf.update()
    threading.Thread(target=updater, daemon=True).start()

def get(url):

    clean_url = url.split('?')[0]
    
    for key in conf.dirs:
        if clean_url[1:].startswith(key):
            return get_file(clean_url[2+len(key)::], key)
    
    return get_file(clean_url[1::], 'html')

def updater():
    while True:
        try:
            conf.update()

            time.sleep(1)
        except(Exception):
            time.sleep(1)
        
