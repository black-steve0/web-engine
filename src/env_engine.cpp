#include "utils.cpp"
#include "class.cpp"
#include <string>
#include <vector>
#include <tuple>
#include <fstream>
#include <filesystem>
#include <expected>
#include <cstdlib>

namespace env {

    static models::Variable _config;
    static bool _loaded = false;

    std::expected<models::Variable, err::Error> parse_value(std::string value) {
        value = utils::strip(value, " ");

        // Quoted string
        if (utils::isString(value, "\"'")) {
            return models::Variable("string", value.substr(1, value.length() - 2));
        }

        // Boolean
        std::string lowered = utils::lowercase(value);

        if (lowered == "true" or lowered == "false") {
            return models::Variable("boolean", lowered);
        }

        // {...} = list
        if (!value.empty() and value.front() == '{' and value.back() == '}') {
            std::string content = utils::strip(value.substr(1, value.length() - 2), " ");

            models::Variable list("list", "");

            if (content.empty()) {
                return list;
            }

            for (const auto& raw_item : utils::split(content, ',')) {
                std::string item = utils::strip(raw_item, " ");
                item = utils::strip(item, "\"'");
                list.addChild(models::Variable("string", item));
            }

            return list;
        }

        // Default: plain, unquoted value - same as Python's fallthrough `return value`
        return models::Variable("string", value);
    }

    // Parses:
    //
    //     "about.html"{--root, default}
    //
    // into a "route" Variable with two named children:
    //     file    -> the parsed file value
    //     options -> the parsed options list
    std::expected<models::Variable, err::Error> parse_route_value(std::string value) {
        value = utils::strip(value, " ");

        auto wrap_no_options = [](std::expected<models::Variable, err::Error> parsed)
            -> std::expected<models::Variable, err::Error> {

            if (!parsed) {
                return std::unexpected(parsed.error());
            }

            models::Variable route;
            route.type = "route";

            models::Variable file = std::move(*parsed);
            file.name = "file";
            route.addChild(std::move(file));

            models::Variable options("list", "");
            options.name = "options";
            route.addChild(std::move(options));

            return route;
        };

        // No options
        if (value.empty() or value.back() != '}') {
            return wrap_no_options(parse_value(value));
        }

        // Find the opening { belonging to the options
        auto options_start = value.rfind('{');

        if (options_start == std::string::npos) {
            return wrap_no_options(parse_value(value));
        }

        std::string file_value = utils::strip(value.substr(0, options_start), " ");
        std::string options_value = utils::strip(value.substr(options_start), " ");

        auto parsed_file = parse_value(file_value);

        if (!parsed_file) {
            return std::unexpected(parsed_file.error());
        }

        auto parsed_options = parse_value(options_value);

        if (!parsed_options) {
            return std::unexpected(parsed_options.error());
        }

        models::Variable route;
        route.type = "route";

        models::Variable file = std::move(*parsed_file);
        file.name = "file";
        route.addChild(std::move(file));

        models::Variable options = std::move(*parsed_options);
        options.name = "options";
        route.addChild(std::move(options));

        return route;
    }

    err::Error load_env(std::filesystem::path filepath) {
        std::ifstream f(filepath);

        if (!f.is_open()) {
            return err::Error(err::Code::OTHER, "Could not open file: " + filepath.string());
        }

        // A frame accumulates the children of the block currently being
        // parsed. We build bottom-up and attach a finished block to its
        // parent's frame on '}', which keeps everything as plain values
        // (no pointers into a vector that could reallocate mid-parse).
        struct Frame {
            std::string name;
            std::vector<models::Variable> children;
        };

        std::vector<Frame> stack;
        stack.push_back(Frame{"", {}});

        std::string line;
        int line_number = 0;

        while (std::getline(f, line)) {
            line_number++;
            line = utils::strip(line, " \t\r\n");

            if (line.empty() or line.starts_with("#") or line.starts_with("//")) {
                continue;
            }

            // Block
            if (line.ends_with("{")) {
                std::string name = utils::strip(line.substr(0, line.length() - 1), " ");

                if (name.empty()) {
                    return err::Error(err::Code::OTHER, "Invalid block on line " + std::to_string(line_number));
                }

                stack.push_back(Frame{name, {}});
                continue;
            }

            // Close block
            if (line == "}") {
                if (stack.size() == 1) {
                    return err::Error(err::Code::OTHER, "Unexpected '}' on line " + std::to_string(line_number));
                }

                Frame finished = std::move(stack.back());
                stack.pop_back();

                models::Variable block;
                block.name = finished.name;
                block.type = "block";
                block.children = std::move(finished.children);

                stack.back().children.push_back(std::move(block));
                continue;
            }

            // Key/value
            auto eq_pos = line.find('=');

            if (eq_pos != std::string::npos) {
                std::string key = utils::strip(line.substr(0, eq_pos), " ");
                std::string value = utils::strip(line.substr(eq_pos + 1), " ");

                // Route syntax:
                // key="file.html"{options}
                bool is_route = false;

                if (!value.empty() and (value.front() == '"' or value.front() == '\'')) {
                    char quote = value.front();
                    auto closing_quote = value.find(quote, 1);

                    if (closing_quote != std::string::npos) {
                        std::string after_quote = utils::strip(value.substr(closing_quote + 1), " ");

                        if (!after_quote.empty() and after_quote.front() == '{') {
                            is_route = true;
                        }
                    }
                }

                auto parsed = is_route ? parse_route_value(value) : parse_value(value);

                if (!parsed) {
                    return parsed.error();
                }

                models::Variable entry = std::move(*parsed);
                entry.name = key;

                stack.back().children.push_back(std::move(entry));
                continue;
            }

            return err::Error(err::Code::OTHER, "Invalid syntax on line " + std::to_string(line_number));
        }

        if (stack.size() != 1) {
            return err::Error(err::Code::OTHER, "Unclosed '{' block");
        }

        models::Variable root;
        root.type = "block";
        root.children = std::move(stack.back().children);

        _config = std::move(root);
        _loaded = true;

        return err::OK;
    }

    std::expected<models::Variable, err::Error> get(std::string variable) {
        if (!_loaded) {
            return std::unexpected(err::Error(err::Code::OTHER, "Config not loaded - call load_env() first"));
        }

        const models::Variable* current = &_config;

        for (const auto& key : utils::split(variable, '.')) {
            const models::Variable* next = current->getChild(key);

            if (next == nullptr) {
                // Fall back to the environment, same as the Python version.
                const char* env_value = std::getenv(variable.c_str());

                if (env_value == nullptr) {
                    return std::unexpected(err::Error(err::Code::UNDEFINED, "Variable not found: " + variable));
                }

                return models::Variable("string", std::string(env_value));
            }

            current = next;
        }

        return *current;
    }
}