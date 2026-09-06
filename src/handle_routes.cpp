#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct Value {
    enum struct Type {
        String,
        Bool,
        List,
        Object
    };

    Type type = Type::String;
    std::string string_value;
    bool bool_value = false;
    std::vector<std::string> list_value;
    std::map<std::string, Value> object_value;
};

extern std::string base;
extern std::string root;
extern bool force_root;

extern std::map<std::string, std::string> dirs;
extern std::map<std::string, Value> dynamic_routes;

void update();

struct RouteResult {
    fs::path file;
    std::vector<std::string> options;
    bool not_found = false;
};

static RouteResult not_found() {
    auto html_it = dirs.find("html");

    if (html_it == dirs.end())
        return {};

    auto routes_it = dynamic_routes.find("html");

    if (routes_it == dynamic_routes.end())
        return {};

    auto route_it =
        routes_it->second.object_value.find("/:");

    if (route_it == routes_it->second.object_value.end())
        return {};

    const Value& value = route_it->second;

    RouteResult result;
    result.not_found = true;

    if (value.type == Value::Type::Object) {
        auto file_it = value.object_value.find("file");
        auto options_it = value.object_value.find("options");

        if (file_it != value.object_value.end() &&
            file_it->second.type == Value::Type::String) {

            result.file =
                fs::path(base) /
                html_it->second /
                file_it->second.string_value;
        }

        if (options_it != value.object_value.end() &&
            options_it->second.type == Value::Type::List) {

            result.options =
                options_it->second.list_value;
        }

        return result;
    }

    if (value.type == Value::Type::String) {
        result.file =
            fs::path(base) /
            html_it->second /
            value.string_value;
    }

    return result;
}

static RouteResult get_file(
    const std::string& route,
    const std::string& type
) {
    auto type_it = dynamic_routes.find(type);

    if (type_it == dynamic_routes.end())
        return not_found();

    auto route_it =
        type_it->second.object_value.find(route);

    if (route_it == type_it->second.object_value.end())
        return not_found();

    const Value& value = route_it->second;

    auto dir_it = dirs.find(type);

    if (dir_it == dirs.end())
        return not_found();

    RouteResult result;

    if (value.type == Value::Type::Object) {
        auto file_it = value.object_value.find("file");
        auto options_it = value.object_value.find("options");

        if (file_it != value.object_value.end() &&
            file_it->second.type == Value::Type::String) {

            result.file =
                fs::path(base) /
                dir_it->second /
                file_it->second.string_value;
        }

        if (options_it != value.object_value.end() &&
            options_it->second.type == Value::Type::List) {

            result.options =
                options_it->second.list_value;
        }
    }
    else if (value.type == Value::Type::String) {
        result.file =
            fs::path(base) /
            dir_it->second /
            value.string_value;
    }

    return result;
}

void init_routes() {
    update();

    std::thread([] {
        while (true) {
            try {
                update();
                std::this_thread::sleep_for(
                    std::chrono::seconds(1)
                );
            }
            catch (...) {
                std::this_thread::sleep_for(
                    std::chrono::seconds(1)
                );
            }
        }
    }).detach();
}

RouteResult get_route(const std::string& url) {
    std::string clean_url = url;

    const auto query = clean_url.find('?');

    if (query != std::string::npos)
        clean_url = clean_url.substr(0, query);

    if (clean_url.empty() || clean_url.front() != '/')
        return not_found();

    for (const auto& [key, directory] : dirs) {
        const std::string prefix = "/" + key;

        if (clean_url.rfind(prefix, 0) == 0) {
            std::string route = clean_url.substr(prefix.size());

            if (!route.empty() && route.front() == '/')
                route.erase(0, 1);

            return get_file(route, key);
        }
    }

    return get_file(clean_url.substr(1), "html");
}