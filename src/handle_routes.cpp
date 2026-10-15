#include "handle_routes.hpp"
#include "conf.hpp"
#include "html_inject.hpp"

#include <fstream>
#include <sstream>

std::expected<std::filesystem::path, err::Error> not_found() {
    const models::Variable* override = conf::defaults.getChild("404");

    if (override != nullptr) {
        auto file = override->getString();

        if (file) {
            return std::filesystem::path(*file);
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
    std::filesystem::path relative = route.relative_path();

    const models::Variable* matchedRoute = conf::routes.getChild(routeKey);

    if (matchedRoute != nullptr) {
        const models::Variable* file = matchedRoute->getChild("file");

        if (file != nullptr) {
            auto fileStr = file->getString();

            if (fileStr) {
                relative = *fileStr;
            }
        }
    }

    const models::Variable* baseDir = conf::dirs.getChild(type);

    if (baseDir == nullptr) {
        return std::unexpected(err::Error(err::Code::OTHER, "Unknown dirs entry: " + type));
    }

    auto baseStr = baseDir->getString();

    if (!baseStr) {
        return std::unexpected(err::Error(err::Code::OTHER, "dirs." + type + " is not a string"));
    }

    return std::filesystem::path(*baseStr) / relative;
}

std::expected<models::Resource, err::Error> getFile(std::filesystem::path route) {
    auto resolved = getFilePath(route, "static");

    if (!resolved) {
        return std::unexpected(resolved.error());
    }

    if (!std::filesystem::exists(*resolved) || std::filesystem::is_directory(*resolved)) {
        return std::unexpected(err::Error(err::Code::OTHER, "File not found: " + resolved->string()));
    }

    if (resolved->extension() == ".html") {
        const models::Variable* matchedRoute = conf::routes.getChild(route.string());
        models::Variable options;

        if (matchedRoute != nullptr) {
            const models::Variable* opts = matchedRoute->getChild("options");

            if (opts != nullptr) {
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