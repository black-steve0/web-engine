#include "env_engine.hpp"
#include "utils.hpp"
#include "class.hpp"
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

        // Default: plain, unquoted value
        return models::Variable("string", value);
    }

    // Parses a single-line value that may carry inline options:
    //
    //     "about.html" {--root, default}
    //
    // The value itself ("about.html") is an ordinary string Variable, exactly
    // like every other path. The inline options become a child list named
    // "options". If there are no inline options, no child is added.
    std::expected<models::Variable, err::Error> parse_entry(std::string value) {
        value = utils::strip(value, " ");

        // Only treat a trailing {...} as options when the value starts with a
        // quoted string, so plain lists like {a, b} still go to parse_value.
        if (!value.empty() and (value.front() == '"' or value.front() == '\'') and value.back() == '}') {
            char quote = value.front();
            auto closing_quote = value.find(quote, 1);

            if (closing_quote != std::string::npos) {
                std::string after_quote = utils::strip(value.substr(closing_quote + 1), " ");

                if (!after_quote.empty() and after_quote.front() == '{') {
                    auto parsed_value = parse_value(value.substr(0, closing_quote + 1));

                    if (!parsed_value) {
                        return std::unexpected(parsed_value.error());
                    }

                    auto parsed_options = parse_value(after_quote);

                    if (!parsed_options) {
                        return std::unexpected(parsed_options.error());
                    }

                    models::Variable entry = std::move(*parsed_value);

                    models::Variable options = std::move(*parsed_options);
                    options.name = "options";
                    entry.addChild(std::move(options));

                    return entry;
                }
            }
        }

        return parse_value(value);
    }

    err::Error load_env(std::filesystem::path filepath) {
        std::ifstream f(filepath);

        if (!f.is_open()) {
            return err::Error(err::Code::OTHER, "Could not open file: " + filepath.string());
        }

        // A frame is a block currently being parsed. `self` is the Variable
        // that will be produced when the block closes: either a plain "block"
        // (for `routes {`) or a value carrying children (for `dirs="/static/" {`).
        // We build bottom-up and attach a finished block to its parent's
        // frame on '}', so everything stays plain values (no pointers into a
        // vector that could reallocate mid-parse).
        struct Frame {
            models::Variable self;
            std::vector<models::Variable> children;
        };

        std::vector<Frame> stack;
        stack.push_back(Frame{models::Variable("block", ""), {}});

        std::string line;
        int line_number = 0;

        while (std::getline(f, line)) {
            line_number++;
            line = utils::strip(line, " \t\r\n");

            if (line.empty() or line.starts_with("#") or line.starts_with("//")) {
                continue;
            }

            // Open block:
            //     name {                -> plain block
            //     name="value" {        -> value with children
            if (line.ends_with("{")) {
                std::string header = utils::strip(line.substr(0, line.length() - 1), " ");

                if (header.empty()) {
                    return err::Error(err::Code::OTHER, "Invalid block on line " + std::to_string(line_number));
                }

                auto eq_pos = header.find('=');

                if (eq_pos == std::string::npos) {
                    models::Variable self("block", "");
                    self.name = header;
                    stack.push_back(Frame{std::move(self), {}});
                } else {
                    std::string key = utils::strip(header.substr(0, eq_pos), " ");
                    std::string value = utils::strip(header.substr(eq_pos + 1), " ");

                    if (value.empty()) {
                        return err::Error(err::Code::OTHER, "Missing value before '{' on line " + std::to_string(line_number));
                    }

                    auto parsed = parse_value(value);

                    if (!parsed) {
                        return parsed.error();
                    }

                    models::Variable self = std::move(*parsed);
                    self.name = key;
                    stack.push_back(Frame{std::move(self), {}});
                }

                continue;
            }

            // Close block
            if (line == "}") {
                if (stack.size() == 1) {
                    return err::Error(err::Code::OTHER, "Unexpected '}' on line " + std::to_string(line_number));
                }

                Frame finished = std::move(stack.back());
                stack.pop_back();

                models::Variable block = std::move(finished.self);
                block.children = std::move(finished.children);

                stack.back().children.push_back(std::move(block));
                continue;
            }

            // Key/value (optionally with inline {options})
            auto eq_pos = line.find('=');

            if (eq_pos != std::string::npos) {
                std::string key = utils::strip(line.substr(0, eq_pos), " ");
                std::string value = utils::strip(line.substr(eq_pos + 1), " ");

                auto parsed = parse_entry(value);

                if (!parsed) {
                    return parsed.error();
                }

                models::Variable entry = std::move(*parsed);
                entry.name = key;

                stack.back().children.push_back(std::move(entry));
                continue;
            }

            return err::Error(err::Code::OTHER, "Invalid syntax in routes.conf on line " + std::to_string(line_number));
        }

        if (stack.size() != 1) {
            return err::Error(err::Code::OTHER, "Unclosed '{' block");
        }

        models::Variable root("block", "");
        root.children = std::move(stack.back().children);

        _config = std::move(root);
        _loaded = true;

        return err::OK;
    }

    std::expected<models::Variable, err::Error> get(std::string variable) {
        if (!_loaded) {
            return std::unexpected(err::Error(err::Code::OTHER, "Config not loaded - call load_env() first"));
        }

        models::Variable current = _config;

        for (const auto& key : utils::split(variable, '.')) {
            auto next = current.getChild(key);

            if (!next) {
                // Fall back to the environment variable.
                const char* env_value = std::getenv(variable.c_str());

                if (!env_value) {
                    return std::unexpected(err::Error(err::Code::UNDEFINED, "Variable not found: " + variable));
                }

                return models::Variable("string", std::string(env_value));
            }

            current = (*next);
        }

        return current;
    }
}