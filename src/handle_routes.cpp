#include "handle_routes.hpp"
#include "conf.hpp"
#include "html_inject.hpp"

#include <cstddef>
#include <expected>
#include <fstream>
#include <sstream>

std::expected<std::filesystem::path, err::Error> not_found() {
    auto override = conf::defaults.getChild("404");

    if (override) {
        auto file = override->getString();

        if (file) {
            std::string basePath = *conf::base.getString();
            std::filesystem::path result = basePath / std::filesystem::path("html/defaults") / *override->getString();

            return result;
        }
    }


    return std::filesystem::path("");
}

// Resolves a request route (e.g. "/about") to a path on disk.
// `type` selects which entry under conf::dirs is used as the base
// directory (e.g. "static"). If the route has a matching entry in
// conf::routes, that entry's "file" value is used instead of the raw
// route.

std::expected<std::filesystem::path, err::Error>
getFilePath(std::filesystem::path route, std::string type) {

    std::string routeKey = route.string();
    std::printf("key thing: %s\n", routeKey.c_str());
    std::printf("type thing: %s\n", type.c_str());

    auto matchedRoute = conf::routes.getChild(type);
    std::printf("got child");
    if (!matchedRoute) {
        std::printf("[getFilePath] type '%s' not found in configuration\n", type.c_str());
        return std::unexpected{err::NOT_FOUND};
    }

    matchedRoute = (*matchedRoute).getChild(routeKey);
    std::printf("got child\n");
    if (!matchedRoute) {
        std::printf("[getFilePath] Route not found in configuration\n");
        return std::unexpected{err::NOT_FOUND};
    }
    std::printf("valid child: %s\n", matchedRoute->name.c_str() );

    std::string basePath = *conf::base.getString();
    auto file = matchedRoute->getString();
    if (!file) {
        matchedRoute->print();

        file = matchedRoute->getChild("file")->getString()->c_str();
        // std::filesystem::path result = basePath / std::filesystem::path(type) / *file;
        
        // return std::unexpected{err::NOT_FOUND};
    }
    std::filesystem::path result = basePath / std::filesystem::path(type) / *file;

    std::printf("formated %s\n", file->c_str());
    return result;
}


std::expected<models::Resource, err::Error> getFile(std::filesystem::path route) {
    std::string url = route.string();

    std::string fragment;
    std::string query;

    if (auto pos = url.find('#'); pos != std::string::npos) {
        fragment = url.substr(pos);
        url = url.substr(0, pos);
    }

    if (auto pos = url.find('?'); pos != std::string::npos) {
        query = url.substr(pos);
        url = url.substr(0, pos);
    }

    std::filesystem::path p(url);

    std::string routeUrl = p.stem().string();
    std::string parent = p.parent_path().string();

    std::printf("route: %s\n", routeUrl.c_str());
    std::printf("type: %s\n", parent.c_str());

    if (parent.empty()) {
        parent = "html";
    } else {
        routeUrl+=p.extension().string();
    }

    std::printf("Resolving path\n");
    auto resolved = getFilePath(routeUrl, parent);

    std::printf("Resolved path\n");
    if (!resolved) {
        return std::unexpected(resolved.error());
    }
    std::printf("Unresolving path\n");

    if (!std::filesystem::exists(*resolved) || std::filesystem::is_directory(*resolved)) {
        return std::unexpected(err::Error(err::Code::OTHER, "File not found: " + resolved->string()));
    }
    std::printf("File found\n");

    if (resolved->extension() == ".html") {
        auto matchedRoute = conf::routes.getChild(route.string());
        models::Variable options;

        if (matchedRoute) {
            auto opts = matchedRoute->getChild("options");

            if (opts) {
                options = *opts;
            }
        }

        return render_html(*resolved, options);
    }

    std::ifstream in(*resolved, std::ios::binary);

    if (!in) {
        return std::unexpected(err::Error(err::Code::OTHER, "Could not open file: " + resolved->string()));
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();

    models::Resource resource;
    resource.route = *resolved;
    resource.content = buffer.str();

    return resource;
}

std::expected<void, err::Error> updater() {
    err::Error e = conf::update();

    if (e.code != static_cast<int>(err::Code::OK)) {
        return std::unexpected(e);
    }

    return {};
}

std::expected<void, err::Error> init() {
    return updater();
}
