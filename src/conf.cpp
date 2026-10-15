#include "conf.hpp"

namespace conf {

    models::Variable dirs;
    models::Variable routes;
    models::Variable base;
    models::Variable mimeTypes;
    models::Variable defaults;
    models::Variable root;

    err::Error update() {
        err::Error loadErr = env::load_env("routes.conf");

        if (loadErr.code != static_cast<int>(err::Code::OK)) {
            return loadErr;
        }

        auto newDirs = env::get("dirs");
        auto newRoutes = env::get("routes");
        auto newBase = env::get("base");
        auto newMimeTypes = env::get("MIME");
        auto newDefaults = env::get("defaults");
        auto newRoot = env::get("force-root");

        if (!newDirs) return newDirs.error();
        if (!newRoutes) return newRoutes.error();
        if (!newBase) return newBase.error();
        if (!newMimeTypes) return newMimeTypes.error();
        if (!newDefaults) return newDefaults.error();
        if (!newRoot) return newRoot.error();

        dirs = *newDirs;
        routes = *newRoutes;
        base = *newBase;
        mimeTypes = *newMimeTypes;
        defaults = *newDefaults;
        root = *newRoot;

        return err::OK;
    }
}