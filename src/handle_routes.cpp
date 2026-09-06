#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace fs = std::filesystem;


// --------------------------------------------------
// Must match Value in env_engine.cpp / conf.cpp
// --------------------------------------------------

struct Value {
    using Object = std::unordered_map<std::string, Value>;
    using Array = std::vector<Value>;

    std::variant<
        std::string,
        bool,
        Array,
        Object
    > data;

    Value() = default;

    Value(const std::string& value)
        : data(value) {}

    Value(const char* value)
        : data(std::string(value)) {}

    Value(bool value)
        : data(value) {}

    Value(const Array& value)
        : data(value) {}

    Value(const Object& value)
        : data(value) {}

    bool is_string() const {
        return std::holds_alternative<std::string>(data);
    }

    bool is_bool() const {
        return std::holds_alternative<bool>(data);
    }

    bool is_array() const {
        return std::holds_alternative<Array>(data);
    }

    bool is_object() const {
        return std::holds_alternative<Object>(data);
    }

    const std::string& as_string() const {
        return std::get<std::string>(data);
    }

    bool as_bool() const {
        return std::get<bool>(data);
    }

    const Array& as_array() const {
        return std::get<Array>(data);
    }

    const Object& as_object() const {
        return std::get<Object>(data);
    }
};


// --------------------------------------------------
// Configuration from conf.cpp
// --------------------------------------------------

extern std::string base;

extern std::map<std::string, std::string> dirs;

extern std::map<std::string, Value> dynamic_routes;


// --------------------------------------------------
// conf.cpp
// --------------------------------------------------

void update();


// --------------------------------------------------
// Result
// --------------------------------------------------

struct RouteResult {
    fs::path file;
    std::vector<std::string> options;
    bool not_found = false;
};


// --------------------------------------------------
// 404 route
// --------------------------------------------------

static RouteResult not_found() {
    auto html_it = dirs.find("html");

    if (html_it == dirs.end())
        return {};

    auto routes_it = dynamic_routes.find("html");

    if (routes_it == dynamic_routes.end())
        return {};

    if (!routes_it->second.is_object())
        return {};

    const auto& routes =
        routes_it->second.as_object();

    auto route_it = routes.find("/:");

    if (route_it == routes.end())
        return {};

    const Value& value = route_it->second;

    RouteResult result;
    result.not_found = true;

    if (value.is_object()) {
        const auto& object = value.as_object();

        auto file_it = object.find("file");

        if (file_it != object.end() &&
            file_it->second.is_string()) {

            result.file =
                fs::path(base) /
                html_it->second /
                file_it->second.as_string();
        }

        auto options_it = object.find("options");

        if (options_it != object.end() &&
            options_it->second.is_array()) {

            for (const auto& option :
                 options_it->second.as_array()) {

                if (option.is_string())
                    result.options.push_back(
                        option.as_string()
                    );
            }
        }

        return result;
    }

    if (value.is_string()) {
        result.file =
            fs::path(base) /
            html_it->second /
            value.as_string();
    }

    return result;
}


// --------------------------------------------------
// Route file
// --------------------------------------------------

static RouteResult get_file(
    const std::string& route,
    const std::string& type
) {
    auto type_it =
        dynamic_routes.find(type);

    if (type_it == dynamic_routes.end())
        return not_found();

    if (!type_it->second.is_object())
        return not_found();

    const auto& routes =
        type_it->second.as_object();

    auto route_it =
        routes.find(route);

    if (route_it == routes.end())
        return not_found();

    auto dir_it =
        dirs.find(type);

    if (dir_it == dirs.end())
        return not_found();

    const Value& value =
        route_it->second;

    RouteResult result;

    if (value.is_object()) {
        const auto& object =
            value.as_object();

        auto file_it =
            object.find("file");

        if (file_it != object.end() &&
            file_it->second.is_string()) {

            result.file =
                fs::path(base) /
                dir_it->second /
                file_it->second.as_string();
        }

        auto options_it =
            object.find("options");

        if (options_it != object.end() &&
            options_it->second.is_array()) {

            for (const auto& option :
                 options_it->second.as_array()) {

                if (option.is_string())
                    result.options.push_back(
                        option.as_string()
                    );
            }
        }
    }
    else if (value.is_string()) {
        result.file =
            fs::path(base) /
            dir_it->second /
            value.as_string();
    }

    return result;
}


// --------------------------------------------------
// Route initialization
// --------------------------------------------------

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


// --------------------------------------------------
// Route lookup
// --------------------------------------------------

RouteResult get_route(
    const std::string& url
) {
    std::string clean_url = url;

    const auto query =
        clean_url.find('?');

    if (query != std::string::npos)
        clean_url =
            clean_url.substr(0, query);

    if (clean_url.empty() ||
        clean_url.front() != '/') {

        return not_found();
    }

    for (const auto& [key, directory] : dirs) {
        const std::string prefix =
            "/" + key;

        if (clean_url.rfind(prefix, 0) == 0) {
            std::string route =
                clean_url.substr(prefix.size());

            if (!route.empty() &&
                route.front() == '/') {

                route.erase(0, 1);
            }

            return get_file(
                route,
                key
            );
        }
    }

    return get_file(
        clean_url.substr(1),
        "html"
    );
}