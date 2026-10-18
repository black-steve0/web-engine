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

// std::expected<std::filesystem::path, err::Error>
// getFilePath(std::filesystem::path route, std::string type) {

//     std::string routeKey = route.string();
//     std::printf("key thing: %s\n", routeKey.c_str());
//     std::printf("type thing: %s\n", type.c_str());

//     auto matchedRoute = conf::routes.getChild(type);
//     std::printf("got child");
//     if (!matchedRoute) {
//         std::printf("[getFilePath] type '%s' not found in configuration\n", type.c_str());
//         return std::unexpected{err::NOT_FOUND};
//     }

//     matchedRoute = (*matchedRoute).getChild(routeKey);
//     std::printf("got child\n");
//     if (!matchedRoute) {
//         std::printf("[getFilePath] Route not found in configuration\n");
//         return std::unexpected{err::NOT_FOUND};
//     }
//     std::printf("valid child: %s\n", matchedRoute->name.c_str() );

//     std::string basePath = *conf::base.getString();
//     auto file = matchedRoute->getString();
//     if (!file) {
//         matchedRoute->print();

//         file = matchedRoute->getChild("file")->getString()->c_str();
//         // std::filesystem::path result = basePath / std::filesystem::path(type) / *file;

//         // return std::unexpected{err::NOT_FOUND};
//     }
//     std::filesystem::path result = basePath / std::filesystem::path(type) / *file;

//     std::printf("formated %s\n", file->c_str());
//     return result;
// }
//
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

std::expected<models::Response, err::Error> get(httplib::Request request) {
    models::Response response;
    response.status = 200;

    DEBUG_LOG(std::printf("Method: %s\n", request.method.c_str()));

    auto url = explodeURL(request.path);
    if (!url) {
        response = errorResponse(err::BAD_REQUEST, "URL is bad.");
        return response;
    }
    DEBUG_LOG(url->print());

    std::string accept = request.get_header_value("Accept");

    DEBUG_LOG(conf::mimeTypes.print());

    // std::string returnType;
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
            if (url->extention == i.name) {
                response.mimeType = *i.getString();
                break;
            }
        }
    }
    if (response.mimeType.empty()) {
        url->extention = "html";
        response.mimeType = "text/html";
    }
    DEBUG_LOG(url->print());

    auto path = getPath(*url);
    if (!path) {
        return std::unexpected{err::INTERNAL_ERROR};
    }
    response.resource.route = *path;

    auto content = renderResource(response.resource.route);
    if (!content) {
        DEBUG_LOG(std::printf("Failed to render resource: %s\n", response.resource.route.c_str()));
        return std::unexpected{err::INTERNAL_ERROR};
    }
    DEBUG_LOG(std::printf("content: %s\n", content->c_str()));

    response.resource.content = *content;

    return response;
}

// resolves URL to path

// std::expected<models::Response, err::Error> getFile(std::filesystem::path route) {
//     models::Response response;
//     std::string url = route.string();

//     std::string fragment;
//     std::string query;

//     if (auto pos = url.find('#'); pos != std::string::npos) {
//         fragment = url.substr(pos);
//         url = url.substr(0, pos);
//     }

//     if (auto pos = url.find('?'); pos != std::string::npos) {
//         query = url.substr(pos);
//         url = url.substr(0, pos);
//     }

//     std::filesystem::path p(url);

//     std::string routeUrl = p.stem().string();
//     std::string parent = p.parent_path().string();

//     std::printf("route: %s\n", routeUrl.c_str());
//     std::printf("type: %s\n", parent.c_str());

//     if (parent.empty()) {
//         parent = "html";
//     } else {
//         routeUrl+=p.extension().string();
//     }

//     std::printf("Resolving path\n");
//     auto resolved = getFilePath(routeUrl, parent);

//     std::printf("Resolved path\n");
//     if (!resolved) {
//         return std::unexpected(resolved.error());
//     }
//     std::printf("Unresolving path\n");

//     if (!std::filesystem::exists(*resolved) || std::filesystem::is_directory(*resolved)) {
//         return std::unexpected(err::Error(err::Code::OTHER, "File not found: " + resolved->string()));
//     }
//     std::printf("File found\n");

//     if (resolved->extension() == ".html") {
//         auto matchedRoute = conf::routes.getChild(route.string());
//         models::Variable options;

//         if (matchedRoute) {
//             auto opts = matchedRoute->getChild("options");

//             if (opts) {
//                 options = *opts;
//             }
//         }

//         return render_html(*resolved, options);
//     }

//     std::ifstream in(*resolved, std::ios::binary);

//     if (!in) {
//         return std::unexpected(err::Error(err::Code::OTHER, "Could not open file: " + resolved->string()));
//     }

//     std::ostringstream buffer;
//     buffer << in.rdbuf();

//     models::Resource resource;
//     resource.route = *resolved;
//     resource.content = buffer.str();

//     return resource;
// }

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
