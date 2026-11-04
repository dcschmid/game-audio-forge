#include "test_main.h"
#include "gaf/config.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

// Paths::resolve must honor GAF_DATA_DIR override.
int test_paths_env_override() {
    setenv("GAF_DATA_DIR", "/tmp/gaf-test-data", 1);
    setenv("GAF_CONFIG_DIR", "/tmp/gaf-test-config", 1);
    setenv("GAF_CACHE_DIR", "/tmp/gaf-test-cache", 1);
    gaf::Paths p = gaf::Paths::resolve();
    int rc = 0;
    if (p.data_dir != "/tmp/gaf-test-data") { std::printf("    data_dir=%s\n", p.data_dir.c_str()); rc = 1; }
    if (p.config_dir != "/tmp/gaf-test-config") { std::printf("    config_dir=%s\n", p.config_dir.c_str()); rc = 1; }
    if (p.cache_dir != "/tmp/gaf-test-cache") { std::printf("    cache_dir=%s\n", p.cache_dir.c_str()); rc = 1; }
    if (p.models_dir() != "/tmp/gaf-test-data/models") { std::printf("    models_dir=%s\n", p.models_dir().c_str()); rc = 1; }
    unsetenv("GAF_DATA_DIR");
    unsetenv("GAF_CONFIG_DIR");
    unsetenv("GAF_CACHE_DIR");
    return rc;
}

// Paths::resolve must fall back to XDG defaults when no override is set.
int test_paths_xdg_default() {
    unsetenv("GAF_DATA_DIR");
    unsetenv("GAF_CONFIG_DIR");
    unsetenv("GAF_CACHE_DIR");
    const char* xdg = std::getenv("XDG_DATA_HOME");
    gaf::Paths p = gaf::Paths::resolve();
    int rc = 0;
    std::string expected;
    if (xdg && *xdg)
        expected = std::string(xdg) + "/game-audio-forge";
    else
        expected = std::string(std::getenv("HOME")) + "/.local/share/game-audio-forge";
    if (p.data_dir != expected) { std::printf("    data_dir=%s expected=%s\n", p.data_dir.c_str(), expected.c_str()); rc = 1; }
    return rc;
}

// Config::load must parse a JSON config with engine + models.
int test_config_load() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-cfg-test";
    fs::create_directories(dir);
    std::string path = dir + "/gaf.json";
    {
        std::ofstream f(path);
        f << R"({
          "engine": {"host": "127.0.0.1", "port": 17495, "server": true,
                     "backends": ["cuda", "cpu"]},
          "models": {
            "ace_step_1_5": {"family": "ace_step", "task": "music",
                             "license": "MIT", "commercial": "true",
                             "default": true, "package": "ace_step_turbo_q8_0"},
            "midashenglm_gen": {"family": "midashenglm_gen", "task": "sfx",
                                "license": "Apache-2.0", "commercial": "true",
                                "default": true, "package": "midashenglm_gen_q8_0"}
          }
        })";
    }
    gaf::Config cfg;
    std::string err;
    if (!gaf::Config::load(path, cfg, err)) { std::printf("    %s\n", err.c_str()); return 1; }
    int rc = 0;
    if (cfg.engine.port != 17495) { std::printf("    port=%d\n", cfg.engine.port); rc = 1; }
    if (cfg.engine.host != "127.0.0.1") { std::printf("    host=%s\n", cfg.engine.host.c_str()); rc = 1; }
    if (cfg.engine.backends.size() != 2 || cfg.engine.backends[0] != "cuda") { std::printf("    backends wrong\n"); rc = 1; }
    if (cfg.models.size() != 2) { std::printf("    models=%zu\n", cfg.models.size()); rc = 1; }
    bool found_ace = false, found_mid = false;
    for (auto& m : cfg.models) {
        if (m.id == "ace_step_1_5") {
            found_ace = true;
            if (m.family != "ace_step" || m.task != "music" || !m.default_model || m.package != "ace_step_turbo_q8_0") {
                std::printf("    ace_step fields wrong\n"); rc = 1;
            }
        }
        if (m.id == "midashenglm_gen") {
            found_mid = true;
            if (m.family != "midashenglm_gen" || m.task != "sfx" || m.license != "Apache-2.0") {
                std::printf("    midashenglm fields wrong\n"); rc = 1;
            }
        }
    }
    if (!found_ace || !found_mid) { std::printf("    missing model entries\n"); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

// Config::load must report an error for a missing file.
int test_config_missing() {
    gaf::Config cfg;
    std::string err;
    if (gaf::Config::load("/tmp/does-not-exist-gaf.json", cfg, err)) {
        std::printf("    expected failure\n");
        return 1;
    }
    if (err.empty()) { std::printf("    expected error message\n"); return 1; }
    return 0;
}

} // namespace

std::vector<Test> config_tests() {
    return {
        {"paths_env_override", test_paths_env_override},
        {"paths_xdg_default", test_paths_xdg_default},
        {"config_load", test_config_load},
        {"config_missing", test_config_missing},
    };
}
