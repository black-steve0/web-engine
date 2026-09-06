#include "httplib.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

// env_engine.cpp
void load_env(const std::string& filename = ".env");
std::string env_get_string(const std::string& key);

// conf.cpp
extern std::map<std::string, std::string> MIME_TYPES;

// handle_routes.cpp
struct RouteResult {
    fs::path file;
    std::vector<std::string> options;
    bool not_found = false;
};

void init_routes();
RouteResult get_route(const std::string& url);

// html_inject.cpp
std::string render_html(
    const fs::path& file_path,
    std::vector<std::string> options
);

static std::string content_type(
    const fs::path& path
) {
    const std::string extension = path.extension().string();

    auto it = MIME_TYPES.find(extension);

    if (it != MIME_TYPES.end())
        return it->second;

    return "application/octet-stream";
}

struct Handler {
    void operator()(
        const httplib::Request& request,
        httplib::Response& response
    ) const {
        RouteResult route = get_route(request.path);

        std::cout << route.file << '\n';

        if (route.file.empty() ||
            !fs::exists(route.file)) {

            response.status = 404;
            response.set_content(
                "404 Not Found",
                "text/plain"
            );
            return;
        }

        std::string body;

        if (route.file.extension() == ".html") {
            body = render_html(
                route.file,
                route.options
            );
        }
        else {
            std::ifstream file(
                route.file,
                std::ios::binary
            );

            if (!file) {
                response.status = 404;
                return;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();

            body = buffer.str();
        }

        response.status =
            route.not_found ? 404 : 200;

        response.set_content(
            body,
            content_type(route.file)
        );
    }
};

int main() {
    load_env("settings.env");

    const std::string port_string =
        env_get_string("port");

    if (port_string.empty()) {
        std::cerr
            << "Missing 'port' in settings.env\n";
        return 1;
    }

    const int port = std::stoi(port_string);

    std::cout
        << "Server started on http://localhost:"
        << port
        << '\n';

    init_routes();

    httplib::Server server;

    server.Get(
        ".*",
        Handler{}
    );

    if (!server.listen("localhost", port)) {
        std::cerr
            << "Failed to start server on port "
            << port
            << '\n';

        return 1;
    }

    return 0;
}