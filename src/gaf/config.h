#pragma once
#include <string>
#include <vector>
#include <optional>
#include <map>

namespace gaf {

// XDG-compatible path resolution.
//
// Resolution order (first match wins):
//   1. Explicit override (env GAF_DATA_DIR / GAF_CONFIG_DIR / GAF_CACHE_DIR)
//   2. XDG base directory (XDG_DATA_HOME / XDG_CONFIG_HOME / XDG_CACHE_HOME)
//   3. Default under $HOME
//
// See docs/DATA-LAYOUT.md for the full contract.
struct Paths {
    std::string data_dir;     // models/, libraries/, projects/, presets/, cache/
    std::string config_dir;   // gaf.json (user config)
    std::string cache_dir;    // scratch, download cache

    // Sub-paths under data_dir.
    std::string models_dir() const { return data_dir + "/models"; }
    std::string libraries_dir() const { return data_dir + "/libraries"; }
    std::string projects_dir() const { return data_dir + "/projects"; }
    std::string presets_dir() const { return data_dir + "/presets"; }
    std::string cache_subdir() const { return data_dir + "/cache"; }

    static Paths resolve();
};

// A model known to the registry (from config [models]).
struct ModelInfo {
    std::string id;           // registry key, e.g. "ace_step_1_5"
    std::string family;       // audio.cpp family, e.g. "ace_step"
    std::string task;         // "music" | "sfx"
    std::string license;
    std::string commercial;   // "true" | "false" | "conditional" | "unknown"
    bool default_model = false;
    std::string package;      // audio.cpp package id, e.g. "ace_step_turbo_q8_0"
};

// Engine / server connection settings.
struct EngineConfig {
    std::string name = "audio.cpp";
    std::vector<std::string> backends = {"cuda", "vulkan", "rocm", "cpu"};
    bool server_enabled = true;
    std::string host = "127.0.0.1";
    int port = 17495;
    std::string base_url() const { return "http://" + host + ":" + std::to_string(port); }
};

// Top-level configuration.
struct Config {
    Paths paths;
    EngineConfig engine;
    std::vector<ModelInfo> models;

    // Load from a JSON config file. Returns false on parse error (err set).
    static bool load(const std::string& path, Config& out, std::string& err);
    // Load from the default user config location (XDG config dir / gaf.json).
    static bool load_default(Config& out, std::string& err);
};

} // namespace gaf
