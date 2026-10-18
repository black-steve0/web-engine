#pragma once

#include "class.hpp"
#include "env_engine.hpp"

namespace conf {

    extern models::Variable dirs;
    extern models::Variable routes;
    extern models::Variable mimeTypes;
    extern models::Variable defaults;
    extern models::Variable root;
    extern models::Variable errors;

    std::expected<void, err::Error> update();

}
