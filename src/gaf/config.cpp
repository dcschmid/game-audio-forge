#include "gaf/config.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

namespace gaf {

namespace {

std::string env_or(const char* name, const std::string& fallback) {
    if (const char* v = std::getenv(name); v && *v) return v;
    return fallback;
}

std::string home_dir() { return env_or("HOME", "/root"); }

// XDG base: $XDG_*_HOME if set and non-empty, else $HOME/.local/share etc.
std::string xdg_base(const char* xdg_var, const std::string& home_default) {
    if (const char* v = std::getenv(xdg_var); v && *v) return v;
    return home_dir() + home_default;
}

} // namespace

Paths Paths::resolve() {
    Paths p;
    p.data_dir = env_or("GAF_DATA_DIR",
                        xdg_base("XDG_DATA_HOME", "/.local/share") + "/game-audio-forge");
    p.config_dir = env_or("GAF_CONFIG_DIR",
                          xdg_base("XDG_CONFIG_HOME", "/.config") + "/game-audio-forge");
    p.cache_dir = env_or("GAF_CACHE_DIR",
                         xdg_base("XDG_CACHE_HOME", "/.cache") + "/game-audio-forge");
    return p;
}

bool Config::load(const std::string& path, Config& out, std::string& err) {
    std::ifstream f(path);
    if (!f) { err = "cannot open config: " + path; return false; }
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        err = std::string("config parse error: ") + e.what();
        return false;
    }

    out.paths = Paths::resolve();
    // Reset engine to defaults so the config file fully describes it.
    out.engine = EngineConfig{};

    {
        auto it = j.find("engine");
        if (it != j.end() && it->is_object()) {
            const auto& e = *it;
            if (e.contains("host")) out.engine.host = e.at("host").get<std::string>();
            if (e.contains("port")) out.engine.port = e.at("port").get<int>();
            if (e.contains("server")) out.engine.server_enabled = e.at("server").get<bool>();
            if (e.contains("backends")) {
                out.engine.backends.clear();
                for (auto& b : e.at("backends")) out.engine.backends.push_back(b.get<std::string>());
            }
        }
    }

    {
        auto it = j.find("models");
        if (it != j.end() && it->is_object()) {
            const auto& m = *it;
            for (auto mit = m.begin(); mit != m.end(); ++mit) {
                ModelInfo mi;
                mi.id = mit.key();
                const auto& v = mit.value();
                if (v.contains("family")) mi.family = v.at("family").get<std::string>();
                if (v.contains("task")) mi.task = v.at("task").get<std::string>();
                if (v.contains("license")) mi.license = v.at("license").get<std::string>();
                if (v.contains("commercial")) mi.commercial = v.at("commercial").get<std::string>();
                if (v.contains("default")) mi.default_model = v.at("default").get<bool>();
                if (v.contains("package")) mi.package = v.at("package").get<std::string>();
                out.models.push_back(std::move(mi));
            }
        }
    }
    return true;
}

bool Config::load_default(Config& out, std::string& err) {
    Paths p = Paths::resolve();
    return load(p.config_dir + "/gaf.json", out, err);
}

} // namespace gaf
