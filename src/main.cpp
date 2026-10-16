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
        auto mime = conf::mimeTypes.getChild(path.extension().string());

        if (!mime) {
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
        return fallback;
    }

    void handleGet(const httplib::Request& req, httplib::Response& res) {
        std::printf("got: %s\n", req.path.c_str());
        auto resource = getFile(req.path.substr(1));

        int status = 200;

        if (!resource) {
            status = resource.error().code;
            resource = readNotFoundResource();
        }

        std::cout << "returned: " << resource->route << std::endl;

        res.status = status;
        res.set_content(resource->content, mimeTypeFor(resource->route));
    }

}

int main() {
    std::printf("Loading settings.env...\n");
    err::Error loadErr = env::load_env("settings.env");
    std::printf("grabbed settings.env...\n");

    if (loadErr.code != static_cast<int>(err::Code::OK)) {
        std::cerr << "Failed to load settings.env: " << loadErr.message << std::endl;
        return 1;
    }
    std::cerr << loadErr.message << std::endl;

    // NOTE: env::load_env() only tracks one global config at a time, so
    // "port" must be read out of settings.env *before* init() loads
    // routes.conf and overwrites it.

    std::printf("attempting to grab port...\n");
    auto portVar = env::get("port");
    std::printf("grabbed port...\n");

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

    std::printf("running init...\n");
    auto initResult = init();

    if (!initResult) {
        std::cerr << "Failed to initialize: " << initResult.error().message << std::endl;
        return 1;
    }

    httplib::Server server;

    std::printf("Get server loading...\n");
    std::printf("Server started on http://localhost:%d\n", port);
    server.Get(".*", handleGet);

    server.listen("localhost", port);

    return 0;
}