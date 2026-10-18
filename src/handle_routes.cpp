#include "handle_routes.hpp"
#include "class.hpp"
#include "conf.hpp"
#include "html_inject.hpp"

#include <expected>
#include <fstream>
#include <sstream>

std::expected<std::string, err::Error> renderResource(std::filesystem::path file_path) {
    std::ifstream in(file_path, std::ios::binary);
    if (!in) {
        return std::unexpected{err::Error(err::Code::INTERNAL_ERROR, "Could not open file: " + file_path.string())};
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

std::expected<std::filesystem::path, err::Error> getPath(models::URL url, err::Error err) {

    auto matchedRoute = conf::routes.getChild(url.extention);
    if (err.code != err::OK.code) {
        matchedRoute = conf::routes.getChild("errors");
    }
    if (!matchedRoute) {
        return std::unexpected{err::NOT_FOUND};
    }

    auto matchedFile = (*matchedRoute).getChild(url.name);
    if (!matchedFile) {
        if (err.code != err::OK.code) {
            matchedFile = (*matchedRoute).getChild("_");
        } else {
            return std::unexpected{err::NOT_FOUND};
        }
        if (!matchedFile) {
            return std::unexpected{err::NOT_FOUND};
        }
    }

    std::string basePath = *conf::base.getString();
    auto file = matchedFile->getString();
    if (!file) {
        file = matchedFile->getChild("file")->getString()->c_str();
    }

    std::filesystem::path result = basePath / std::filesystem::path(matchedRoute->name) / *file;

    return result;
}

models::Response errorResponse(err::Error err, std::string message) {
    models::Response response;
    response.status = err.code;

    httplib::Request request;
    request.path = "/errors/" + std::to_string(err.code);
    request.set_header("Accept", "text/html");

    auto res = get(request, err);
    if (!res) {
        response.status = res.error().code;
        response.mimeType = "application/octet-stream";
        return response;
    }
    response = *res;

    return response;
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

// game plan: Resolves entire page
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
std::expected<models::Response, err::Error> get(httplib::Request request, err::Error err) {
    models::Response response;
    response.status = err.code;

    auto url = explodeURL(request.path);
    if (!url) {
        response = errorResponse(err::BAD_REQUEST, "URL is bad.");
        return response;
    }

    std::string accept = request.get_header_value("Accept");
    auto mimeResult = injectMime(response, *url, accept);
    if (!mimeResult) {
        if (response.status == static_cast<unsigned int>(err::INTERNAL_ERROR.code)) {
            return std::unexpected{err::INTERNAL_ERROR};
        }
        response = errorResponse(mimeResult.error(), "Route not found");
        return response;
    }
    auto path = getPath(*url, err);
    if (!path) {
        if (response.status == static_cast<unsigned int>(err::INTERNAL_ERROR.code)) {
            return std::unexpected{err::INTERNAL_ERROR};
        }
        response = errorResponse(path.error(), "Unable to aquire path");
        return response;
    }
    response.resource.route = *path;

    auto content = renderResource(response.resource.route);
    if (!content) {
        
        if (response.status == static_cast<unsigned int>(err::INTERNAL_ERROR.code)) {
            return std::unexpected{err::INTERNAL_ERROR};
        }
        response = errorResponse(content.error(), "cant render resource");
        return response;
    }

    response.resource.content = *content;

    return response;
}

std::expected<void, err::Error> updater() {
    auto e = conf::update();

    if (!e) {
        return std::unexpected(e.error());
    }

    return {};
}

std::expected<void, err::Error> init() {
    return updater();
}
