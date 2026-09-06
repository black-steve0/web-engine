#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

struct Value {
    enum class Type {
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

static std::map<std::string, Value> config;

static std::string trim(const std::string& s) {
    const auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return "";

    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static Value parse_value(const std::string& input) {
    std::string value = trim(input);

    // Quoted string
    if (value.size() >= 2 &&
        value.front() == value.back() &&
        (value.front() == '"' || value.front() == '\'')) {

        Value result;
        result.type = Value::Type::String;
        result.string_value = value.substr(1, value.size() - 2);
        return result;
    }

    // Boolean
    if (value == "true" || value == "TRUE" ||
        value == "True") {

        Value result;
        result.type = Value::Type::Bool;
        result.bool_value = true;
        return result;
    }

    if (value == "false" || value == "FALSE" ||
        value == "False") {

        Value result;
        result.type = Value::Type::Bool;
        result.bool_value = false;
        return result;
    }

    // List
    if (value.size() >= 2 &&
        value.front() == '{' &&
        value.back() == '}') {

        Value result;
        result.type = Value::Type::List;

        std::string content =
            trim(value.substr(1, value.size() - 2));

        if (content.empty())
            return result;

        std::stringstream ss(content);
        std::string item;

        while (std::getline(ss, item, ',')) {
            item = trim(item);

            if (item.size() >= 2 &&
                ((item.front() == '"' && item.back() == '"') ||
                 (item.front() == '\'' && item.back() == '\''))) {

                item = item.substr(1, item.size() - 2);
            }

            result.list_value.push_back(item);
        }

        return result;
    }

    Value result;
    result.type = Value::Type::String;
    result.string_value = value;
    return result;
}

static Value parse_route_value(const std::string& input) {
    std::string value = trim(input);

    if (value.empty() || value.back() != '}') {
        return parse_value(value);
    }

    const auto options_start = value.rfind('{');

    if (options_start == std::string::npos) {
        return parse_value(value);
    }

    const std::string file_value =
        trim(value.substr(0, options_start));

    const std::string options_value =
        trim(value.substr(options_start));

    Value result;
    result.type = Value::Type::Object;

    result.object_value["file"] = parse_value(file_value);
    result.object_value["options"] = parse_value(options_value);

    return result;
}

void load_env(const std::string& filename = ".env") {
    std::ifstream file(filename);

    if (!file)
        throw std::runtime_error("Could not open config file: " + filename);

    std::string line;
    int line_number = 0;

    std::vector<std::map<std::string, Value>*> stack;
    stack.push_back(&config);

    while (std::getline(file, line)) {
        ++line_number;

        line = trim(line);

        if (line.empty() ||
            line.rfind("#", 0) == 0 ||
            line.rfind("//", 0) == 0) {
            continue;
        }

        // Block
        if (line.back() == '{') {
            std::string name =
                trim(line.substr(0, line.size() - 1));

            if (name.empty()) {
                throw std::runtime_error(
                    "Invalid block on line " +
                    std::to_string(line_number));
            }

            Value block;
            block.type = Value::Type::Object;

            (*stack.back())[name] = block;

            stack.push_back(
                &(*stack.back())[name].object_value
            );

            continue;
        }

        // Close block
        if (line == "}") {
            if (stack.size() == 1) {
                throw std::runtime_error(
                    "Unexpected '}' on line " +
                    std::to_string(line_number));
            }

            stack.pop_back();
            continue;
        }

        // Key/value
        const auto equals = line.find('=');

        if (equals != std::string::npos) {
            std::string key =
                trim(line.substr(0, equals));

            std::string value =
                trim(line.substr(equals + 1));

            // Route syntax:
            // key="file.html"{--root, default}
            if (!value.empty() &&
                (value.front() == '"' || value.front() == '\'')) {

                const char quote = value.front();
                const auto closing_quote =
                    value.find(quote, 1);

                if (closing_quote != std::string::npos) {
                    std::string after_quote =
                        trim(value.substr(closing_quote + 1));

                    if (!after_quote.empty() &&
                        after_quote.front() == '{') {

                        (*stack.back())[key] =
                            parse_route_value(value);

                        continue;
                    }
                }
            }

            (*stack.back())[key] = parse_value(value);
            continue;
        }

        throw std::runtime_error(
            "Invalid syntax on line " +
            std::to_string(line_number));
    }

    if (stack.size() != 1) {
        throw std::runtime_error("Unclosed '{' block");
    }
}

static const Value* find_value(const std::string& path) {
    const Value* current = nullptr;

    std::stringstream ss(path);
    std::string key;

    const std::map<std::string, Value>* object = &config;

    while (std::getline(ss, key, '.')) {
        auto it = object->find(key);

        if (it == object->end())
            return nullptr;

        current = &it->second;

        if (current->type != Value::Type::Object)
            break;

        object = &current->object_value;
    }

    return current;
}

std::string env_get_string(const std::string& key) {
    const Value* value = find_value(key);

    if (value) {
        if (value->type == Value::Type::String)
            return value->string_value;

        if (value->type == Value::Type::Bool)
            return value->bool_value ? "true" : "false";
    }

    const char* env = std::getenv(key.c_str());

    if (env)
        return env;

    return "";
}

bool env_get_bool(const std::string& key) {
    const Value* value = find_value(key);

    if (value && value->type == Value::Type::Bool)
        return value->bool_value;

    const std::string fallback = env_get_string(key);

    return fallback == "true" ||
           fallback == "TRUE" ||
           fallback == "True";
}

std::vector<std::string> env_get_list(const std::string& key) {
    const Value* value = find_value(key);

    if (value && value->type == Value::Type::List)
        return value->list_value;

    return {};
}

std::map<std::string, Value> env_get_object(const std::string& key) {
    const Value* value = find_value(key);

    if (value && value->type == Value::Type::Object)
        return value->object_value;

    return {};
}