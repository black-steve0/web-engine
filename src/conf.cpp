#include "class.cpp"
#include "env_engine.cpp"

namespace conf{

    models::Variable dirs;
    models::Variable routes;
    models::Variable base;
    models::Variable mimeTypes;
    models::Variable defaults;
    models::Variable root;

    err::Error update() {
        env::load_env("routes.conf");

        auto newDirs = env::get("dirs");
        auto newRoutes = env::get("routes");
        auto newBase = env::get("base");
        auto newMimeTypes = env::get("MIME");
        auto newDefaults = env::get("defaults");
        auto newRoot = env::get("force-root");

        if (!newDirs || !newRoutes || !newBase || !newMimeTypes ||
            !newDefaults || !newRoot) {
            if (!newDirs) return newDirs.error();
            if (!newRoutes) return newRoutes.error();
            if (!newBase) return newBase.error();
            if (!newMimeTypes) return newMimeTypes.error();
            if (!newDefaults) return newDefaults.error();
            if (!newRoot) return newRoot.error();
        }

        dirs = *newDirs;
        routes = *newRoutes;
        base = *newBase;
        mimeTypes = *newMimeTypes;
        defaults = *newDefaults;
        root = *newRoot;

        return err::OK;
    }
}