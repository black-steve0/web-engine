#include "handle_routes.hpp"
#include "class.hpp"
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

std::expected<std::string, err::Error> renderResource(std::filesystem::path file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        return std::unexpected{err::Error(err::Code::OTHER, "Could not open file: " + file_path.string())};
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();

    return buffer.str();
}

std::expected<models::URL, err::Error> explodeURL(std::filesystem::path URL) {
    models::URL result;

    std::string url = URL.string();
    std::filesystem::path p(url);

    result.name = p.stem().string();
    result.parent = p.parent_path().string();

    if (auto pos = url.find('.'); pos != std::string::npos) {
        result.extention = url.substr(pos+1);
        url = url.substr(0, pos);

        result.name += "."+result.extention;
    }

    if (auto pos = url.find('#'); pos != std::string::npos) {
        result.fragment = url.substr(pos);
        url = url.substr(0, pos);
    }

    if (auto pos = url.find('?'); pos != std::string::npos) {
        result.query = url.substr(pos);
        url = url.substr(0, pos);
    }

    return result;
}

std::expected<std::filesystem::path, err::Error> getPath(models::URL url) {
    auto matchedRoute = conf::routes.getChild(url.extention);
    if (!matchedRoute) {
        DEBUG_LOG(std::printf("[getFilePath] type '%s' not found in configuration\n", url.extention.c_str()));
        return std::unexpected{err::NOT_FOUND};
    }

    matchedRoute = (*matchedRoute).getChild(url.name);
    if (!matchedRoute) {
        DEBUG_LOG(std::printf("[getFilePath] Route not found in configuration\n"));
        return std::unexpected{err::NOT_FOUND};
    }

    std::string basePath = *conf::base.getString();
    auto file = matchedRoute->getString();
    if (!file) {
        matchedRoute->print();
        file = matchedRoute->getChild("file")->getString()->c_str();
    }

    std::filesystem::path result = basePath / std::filesystem::path(url.extention) / *file;

    return result;
}

// Resolves entire page
//
// deal middleware??
// todo
//
// gets the actual page/api/else: about, index, etc
//
    // indetifies 404
    // gets 404 path
    // sets page/api to 404 page/api
//
// identifies api or not
// api:??
// todo
//
// not api:
// gets the type
// gets the file path
//
// gets options recursively
// gets options modifiers
// gets options paths
// injects options
//
// gets the query: ?id=123
// resolves query??
// todo
//

models::Response errorResponse(err::Error, std::string message) {
    models::Response reponse;



    return reponse;
}

std::expected<void, err::Error> injectMime(models::Response& response, models::URL& url, std::string accept) {
    for (auto i: conf::mimeTypes.getChildren()) {
        if (!i.getString()) {
            return std::unexpected{err::INTERNAL_ERROR};
        }
        if (accept == i.getString()) {
            response.mimeType = *i.getString();
            break;
        }
    }
    if (response.mimeType.empty()) {
        for (auto i: conf::mimeTypes.getChildren()) {
            if (!i.getString()) {
                return std::unexpected{err::INTERNAL_ERROR};
            }
            if (url.extention == i.name) {
                response.mimeType = *i.getString();
                break;
            }
        }
    }
    if (response.mimeType.empty()) {
        url.extention = "html";
        response.mimeType = "text/html";
    }
    
    return {};
};

std::expected<models::Response, err::Error> get(httplib::Request request) {
    models::Response response;
    response.status = 200;

    auto url = explodeURL(request.path);
    if (!url) {
        response = errorResponse(err::BAD_REQUEST, "URL is bad.");
        return response;
    }

    std::string accept = request.get_header_value("Accept");
    auto mimeResult = injectMime(response, *url, accept);
    if (!mimeResult) {
        return std::unexpected{err::INTERNAL_ERROR};
    }

    auto path = getPath(*url);
    if (!path) {
        return std::unexpected{err::INTERNAL_ERROR};
    }
    response.resource.route = *path;

    auto content = renderResource(response.resource.route);
    if (!content) {
        return std::unexpected{err::INTERNAL_ERROR};
    }

    response.resource.content = *content;

    return response;
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
