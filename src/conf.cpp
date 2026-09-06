#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <variant>
#include <stdexcept>
#include <utility>

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


// Functions from env_engine.cpp

void load_env(const std::string& filename);

const Value* get_value(const std::string& path);

std::string get(const std::string& name);


// --------------------------------------------------
// Existing configuration expected by old code
// --------------------------------------------------

std::map<std::string, Value> dynamic_routes;

std::map<std::string, std::string> dirs;

std::map<std::string, std::string> MIME_TYPES;

std::map<std::string, std::string> defaults;

std::string base;

bool force_root = false;


// --------------------------------------------------
// New API configuration
// --------------------------------------------------

struct ApiRoute {
    std::string name;
    std::string method;
    std::string path;
    std::string runtime;
    std::string mode;
    std::string entry;
};

std::vector<ApiRoute> api_routes;


// --------------------------------------------------
// Helpers
// --------------------------------------------------

static std::string get_string(
    const Value::Object& object,
    const std::string& key
) {
    auto it = object.find(key);

    if (it == object.end())
        return "";

    if (!it->second.is_string()) {
        throw std::runtime_error(
            "Configuration value '" +
            key +
            "' must be a string"
        );
    }

    return it->second.as_string();
}


// --------------------------------------------------
// Configuration update
// --------------------------------------------------

void update() {
    load_env("routes.conf");

    dynamic_routes.clear();
    dirs.clear();
    MIME_TYPES.clear();
    defaults.clear();
    api_routes.clear();

    base = get("base");

    force_root = get("force-root") == "true";


    // --------------------------------------------------
    // dirs
    // --------------------------------------------------

    if (const Value* value = get_value("dirs")) {

        if (!value->is_object()) {
            throw std::runtime_error(
                "'dirs' must be a block"
            );
        }

        for (const auto& [key, val] :
             value->as_object()) {

            if (val.is_string())
                dirs[key] = val.as_string();
        }
    }


    // --------------------------------------------------
    // defaults
    // --------------------------------------------------

    if (const Value* value =
        get_value("defaults")) {

        if (!value->is_object()) {
            throw std::runtime_error(
                "'defaults' must be a block"
            );
        }

        for (const auto& [key, val] :
             value->as_object()) {

            if (val.is_string())
                defaults[key] = val.as_string();
        }
    }


    // --------------------------------------------------
    // MIME
    // --------------------------------------------------

    if (const Value* value =
        get_value("MIME")) {

        if (!value->is_object()) {
            throw std::runtime_error(
                "'MIME' must be a block"
            );
        }

        for (const auto& [key, val] :
             value->as_object()) {

            if (val.is_string())
                MIME_TYPES[key] = val.as_string();
        }
    }


    // --------------------------------------------------
    // Routes
    // --------------------------------------------------

    if (const Value* value =
        get_value("routes")) {

        if (!value->is_object()) {
            throw std::runtime_error(
                "'routes' must be a block"
            );
        }

        for (const auto& [type, route_value] :
             value->as_object()) {

            dynamic_routes[type] = route_value;
        }
    }


    // --------------------------------------------------
    // API
    // --------------------------------------------------

    if (const Value* value =
        get_value("api")) {

        if (!value->is_object()) {
            throw std::runtime_error(
                "'api' must be a block"
            );
        }

        for (const auto& [name, route] :
             value->as_object()) {

            if (!route.is_object()) {
                throw std::runtime_error(
                    "API route '" +
                    name +
                    "' must be a block"
                );
            }

            const auto& object =
                route.as_object();

            ApiRoute api;

            api.name = name;
            api.method = get_string(object, "method");
            api.path = get_string(object, "path");
            api.runtime = get_string(object, "runtime");
            api.mode = get_string(object, "mode");
            api.entry = get_string(object, "entry");

            if (api.method.empty())
                api.method = "GET";

            if (api.mode.empty())
                api.mode = "exec";

            if (api.path.empty()) {
                throw std::runtime_error(
                    "API route '" +
                    name +
                    "' has no path"
                );
            }

            if (api.runtime.empty()) {
                throw std::runtime_error(
                    "API route '" +
                    name +
                    "' has no runtime"
                );
            }

            if (api.entry.empty()) {
                throw std::runtime_error(
                    "API route '" +
                    name +
                    "' has no entry"
                );
            }

            api_routes.push_back(
                std::move(api)
            );
        }
    }
}


// --------------------------------------------------
// Compatibility with existing main.cpp
// --------------------------------------------------

std::string env_get_string(
    const std::string& name
) {
    return get(name);
}