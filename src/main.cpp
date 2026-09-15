#include "httplib.h"

int main() {
    httplib::Server server;

    server.Get("/", [](const httplib::Request& req, httplib::Response& res) {
        res.set_content(
            "<h1>Hello</h1>",
            "text/html"
        );
    });

    server.listen("localhost", 8080);
}