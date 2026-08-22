from http.server import HTTPServer, BaseHTTPRequestHandler
from pathlib import Path
from urllib.parse import urlparse, parse_qs
import os
import json

import env_engine
import conf
import handle_routes

env_engine.load_env("settings.env")

class Handler(BaseHTTPRequestHandler):

    # ---- helpers ----------------------------------------------------

    def _send_json(self, data, status=200):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_json(self):
        length = int(self.headers.get("Content-Length", 0) or 0)
        if length == 0:
            return {}
        raw = self.rfile.read(length)
        try:
            return json.loads(raw)
        except json.JSONDecodeError:
            return {}

    @staticmethod
    def _path_parts(path):
        return [p for p in urlparse(path).path.split("/") if p]

    # ---- static file routes ------------------------------------------

    def do_GET(self):
        parts = self._path_parts(self.path)

        handler = handle_routes.get(self.path)
        file_path = handler()

        if file_path.exists():
            status = 404 if handler == handle_routes.not_found else 200
            self.send_response(status)
            self.send_header(
                "Content-Type",
                conf.MIME_TYPES.get(file_path.suffix, "application/octet-stream")
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