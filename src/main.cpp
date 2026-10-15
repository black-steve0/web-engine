#include "httplib.h"

#include "class.hpp"
#include "env_engine.hpp"
#include "conf.hpp"
#include "handle_routes.hpp"

#include <fstream>
#include <iostream>
#include <sstream>

namespace {

    std::string mimeTypeFor(const std::filesystem::path& path) {
        const models::Variable* mime = conf::mimeTypes.getChild(path.extension().string());

        if (mime == nullptr) {
            return "application/octet-stream";
        }

        auto value = mime->getString();

        return value ? *value : "application/octet-stream";
    }

    models::Resource readNotFoundResource() {
        auto fallbackPath = not_found();

        models::Resource fallback;
        fallback.route = fallbackPath ? *fallbackPath : std::filesystem::path("");

        if (!fallback.route.empty() && std::filesystem::exists(fallback.route)) {
            std::ifstream in(fallback.route, std::ios::binary);
            std::ostringstream buffer;
            buffer << in.rdbuf();
            fallback.content = buffer.str();
        } else {
            fallback.content = "404 Not Found";
        }
        std::printf("%s", fallback.content.c_str());
        return fallback;
    }

    void handleGet(const httplib::Request& req, httplib::Response& res) {
        auto resource = getFile(req.path);

        int status = 200;

        if (!resource) {
            status = 404;
            resource = readNotFoundResource();
        }

        std::cout << resource->route << std::endl;

        res.status = status;
        res.set_content(resource->content, mimeTypeFor(resource->route));
    }

}

int main() {
    err::Error loadErr = env::load_env("settings.env");

    if (loadErr.code != static_cast<int>(err::Code::OK)) {
        std::cerr << "Failed to load settings.env: " << loadErr.message << std::endl;
        return 1;
    }

    // NOTE: env::load_env() only tracks one global config at a time, so
    // "port" must be read out of settings.env *before* init() loads
    // routes.conf and overwrites it.
    auto portVar = env::get("port");

    if (!portVar) {
        std::cerr << "port is not defined in settings.env" << std::endl;
        return 1;
    }

    auto portStr = portVar->getString();

    if (!portStr) {
        std::cerr << "port must be a string value" << std::endl;
        return 1;
    }

    int port = std::stoi(*portStr);

    auto initResult = init();

    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().message << std::endl;
        return 1;
    }

    httplib::Server server;

    server.Get(".*", handleGet);

    std::printf("Server started on http://localhost:%d", port);

    server.listen("localhost", port);

    return 0;
}