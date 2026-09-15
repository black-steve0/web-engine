#include "utils.cpp"
#include "class.cpp"
#include <string>
#include <tuple>
#include <expected>

namespace env {


    std::expected<models::Variable, err::Error> parse_value(std::string value) {
        value = utils::strip(value, ' ');

        if (utils::isString(value, "\"\'\n")) {
            return models::Variable("string", value.substr(1,value.length()-2));
        }

        if (utils::lowercase(value) == "true" or utils::lowercase(value) == "false") {
            return models::Variable("boolean", utils::lowercase(value));
        }

        if (value.front() == '{' and value.back() == '{') {

        }

        return std::unexpected(err::Error(err::Code::OTHER, "Couldn't find a suitable type for the input"));
    }

    std::expected<std::string, err::Error> parseRouteValue(std::string value) {
        value = utils::strip(value, ' ');

        if (value.back() != '}') {
            std::string name;

            auto parsed_value = parse_value(value);

            if (!parsed_value) {
                return std::unexpected(parsed_value.error());
            }


            name = *(*parsed_value).getString();

            return name;
        }
    }

    err::Error load_env(std::filesystem::path filepath) {

    }

    std::expected<models::Variable, err::Error> get(std::string varaible) {

    }
}