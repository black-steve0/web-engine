#include "conf.hpp"
#include "class.hpp"

namespace conf {

    models::Variable force_root;
    models::Variable dirs;
    models::Variable routes;
    models::Variable mimeTypes;
    models::Variable defaults;
    models::Variable root;

    std::expected<void, err::Error> update() {
        err::Error loadErr = env::load_env("routes.conf");

        if (loadErr.code != static_cast<int>(err::Code::OK)) {
            return std::unexpected{loadErr};
        }

        auto force_root = env::get("force-root");
        auto newDirs = env::get("dirs");
        auto newRoutes = env::get("routes");
        auto newMimeTypes = env::get("MIME");
        auto newDefaults = env::get("defaults");
        auto newRoot = env::get("force-root");

        if (!force_root) return std::unexpected{force_root.error()};
        if (!newDirs) return std::unexpected{newDirs.error()};
        if (!newRoutes) return std::unexpected{newRoutes.error()};
        if (!newMimeTypes) return std::unexpected{newMimeTypes.error()};
        if (!newDefaults) return std::unexpected{newDefaults.error()};

        dirs = *newDirs;
        routes = *newRoutes;
        mimeTypes = *newMimeTypes;
        defaults = *newDefaults;
        root = *newRoot;

        return {};
    }
}
