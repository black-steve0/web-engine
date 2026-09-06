#include <map>
#include <string>
#include <vector>

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

void load_env(const std::string& filename);

std::string env_get_string(const std::string& key);
bool env_get_bool(const std::string& key);
std::vector<std::string> env_get_list(const std::string& key);
std::map<std::string, Value> env_get_object(const std::string& key);

std::string base;
std::string root;
bool force_root = false;

std::map<std::string, std::string> dirs;
std::map<std::string, Value> dynamic_routes;
std::map<std::string, std::string> MIME_TYPES;
std::map<std::string, std::string> defaults;

void update() {
    load_env("routes.conf");

    dirs.clear();
    dynamic_routes.clear();
    MIME_TYPES.clear();
    defaults.clear();

    base = env_get_string("base");
    force_root = env_get_bool("force-root");

    {
        auto values = env_get_object("dirs");

        for (const auto& [key, value] : values) {
            if (value.type == Value::Type::String)
                dirs[key] = value.string_value;
        }
    }

    {
        auto values = env_get_object("routes");

        dynamic_routes = values;
    }

    {
        auto values = env_get_object("MIME");

        for (const auto& [key, value] : values) {
            if (value.type == Value::Type::String)
                MIME_TYPES[key] = value.string_value;
        }
    }

    {
        auto values = env_get_object("defaults");

        for (const auto& [key, value] : values) {
            if (value.type == Value::Type::String)
                defaults[key] = value.string_value;
        }
    }
}