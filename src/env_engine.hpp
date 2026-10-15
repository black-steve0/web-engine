#pragma once

#include "class.hpp"
#include <filesystem>
#include <expected>
#include <string>

namespace env {

    std::expected<models::Variable, err::Error>
    parse_value(std::string value);

    std::expected<models::Variable, err::Error>
    parse_route_value(std::string value);

    err::Error load_env(std::filesystem::path filepath);

    std::expected<models::Variable, err::Error>
    get(std::string variable);

}