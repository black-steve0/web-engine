#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>
#include <cstdlib>
#include <stdexcept>
#include <algorithm>

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

    std::string as_string() const {
        if (!is_string())
            throw std::runtime_error("Value is not a string");

        return std::get<std::string>(data);
    }

    bool as_bool() const {
        if (!is_bool())
            throw std::runtime_error("Value is not a boolean");

        return std::get<bool>(data);
    }

    const Array& as_array() const {
        if (!is_array())
            throw std::runtime_error("Value is not an array");

        return std::get<Array>(data);
    }

    const Object& as_object() const {
        if (!is_object())
            throw std::runtime_error("Value is not an object");

        return std::get<Object>(data);
    }
};

static std::unordered_map<std::string, Value> config;

static std::string trim(const std::string& input) {
    const auto first = input.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = input.find_last_not_of(" \t\r\n");

    return input.substr(first, last - first + 1);
}

static Value parse_value(const std::string& raw) {
    std::string value = trim(raw);

    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') ||
         (value.front() == '\'' && value.back() == '\''))) {

        return Value(value.substr(1, value.size() - 2));
    }

    std::string lower = value;

    std::transform(
        lower.begin(),
        lower.end(),
        lower.begin(),
        [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        }
    );

    if (lower == "true")
        return Value(true);

    if (lower == "false")
        return Value(false);

    if (value.size() >= 2 &&
        value.front() == '{' &&
        value.back() == '}') {

        std::string content =
            trim(value.substr(1, value.size() - 2));

        Value::Array array;

        if (content.empty())
            return Value(array);

        std::stringstream stream(content);
        std::string item;

        while (std::getline(stream, item, ',')) {
            item = trim(item);

            if (item.size() >= 2 &&
                ((item.front() == '"' && item.back() == '"') ||
                 (item.front() == '\'' && item.back() == '\''))) {

                item = item.substr(1, item.size() - 2);
            }

            array.emplace_back(item);
        }

        return Value(array);
    }

    return Value(value);
}

void load_env(const std::string& filename) {
    std::ifstream file(filename);

    if (!file)
        throw std::runtime_error(
            "Could not open configuration file: " + filename
        );

    Value::Object root;
    std::vector<Value::Object*> stack;

    stack.push_back(&root);

    std::string line;
    int line_number = 0;

    while (std::getline(file, line)) {
        ++line_number;

        line = trim(line);

        if (line.empty())
            continue;

        if (line.starts_with("#") ||
            line.starts_with("//")) {
            continue;
        }

        if (line.ends_with("{")) {
            std::string name =
                trim(line.substr(0, line.size() - 1));

            if (name.empty()) {
                throw std::runtime_error(
                    "Invalid block on line " +
                    std::to_string(line_number)
                );
            }

            Value::Object new_block;

            (*stack.back())[name] = Value(new_block);

            auto& object =
                std::get<Value::Object>(
                    (*stack.back())[name].data
                );

            stack.push_back(&object);

            continue;
        }

        if (line == "}") {
            if (stack.size() == 1) {
                throw std::runtime_error(
                    "Unexpected '}' on line " +
                    std::to_string(line_number)
                );
            }

            stack.pop_back();
            continue;
        }

        auto equals = line.find('=');

        if (equals != std::string::npos) {
            std::string key =
                trim(line.substr(0, equals));

            std::string value =
                trim(line.substr(equals + 1));

            (*stack.back())[key] = parse_value(value);

            continue;
        }

        throw std::runtime_error(
            "Invalid syntax on line " +
            std::to_string(line_number)
        );
    }

    if (stack.size() != 1) {
        throw std::runtime_error(
            "Unclosed '{' block"
        );
    }

    config = std::move(root);
}

const Value* get_value(const std::string& path) {
    const Value::Object* current = &config;

    std::stringstream stream(path);
    std::string key;

    while (std::getline(stream, key, '.')) {
        auto it = current->find(key);

        if (it == current->end())
            return nullptr;

        if (stream.peek() == EOF)
            return &it->second;

        if (!it->second.is_object())
            return nullptr;

        current = &it->second.as_object();
    }

    return nullptr;
}

std::string get(const std::string& name) {
    const Value* value = get_value(name);

    if (value && value->is_string())
        return value->as_string();

    if (value && value->is_bool())
        return value->as_bool() ? "true" : "false";

    const char* environment = std::getenv(name.c_str());

    if (environment)
        return environment;

    return "";
}