from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs
import os

import env_engine
import conf
import handle_routes

env_engine.load_env("settings.env")

BASE_DIR = Path(__file__).parent

MIME_TYPES = {
    ".html": "text/html",
    ".css": "text/css",
    ".js": "application/javascript",
    ".png": "image/png",
}

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        handler = handle_routes.get(self.path)
        file_path = handler()

        if file_path.exists():
            status = 404 if handler == handle_routes.not_found else 200

            self.send_response(status)
            self.send_header(
                "Content-Type",
                MIME_TYPES.get(
                    file_path.suffix,
                    "application/octet-stream"
                )
            )
            self.end_headers()
            self.wfile.write(file_path.read_bytes())
        else:
            self.send_error(404)


if __name__ == "__main__":
    port = env_engine.get("port")
    print("Server started on http://localhost:" + port)

    handle_routes.init()

    HTTPServer(("localhost", int(port)), Handler).serve_forever()