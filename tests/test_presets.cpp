#include "test_main.h"
#include "gaf/presets.h"

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {

int test_preset_save_find() {
    namespace fs = std::filesystem;
    std::string user = "/tmp/gaf-preset-user";
    fs::remove_all(user);
    fs::create_directories(user);
    std::string shipped = "/tmp/gaf-preset-shipped";
    fs::remove_all(shipped);
    fs::create_directories(shipped);

    gaf::Presets store(shipped, user);
    std::string err;
    gaf::Preset p;
    p.name = "my-sfx";
    p.kind = "sfx";
    p.model = "midashenglm_gen";
    p.prompt = "a heavy metal door slam";
    p.duration_s = 2.0;
    p.seed = 42;
    p.tags = {"door", "slam"};
    if (!store.save(p, err)) { std::printf("    save: %s\n", err.c_str()); return 1; }

    gaf::Preset got = store.find("my-sfx");
    int rc = 0;
    if (got.name.empty()) { std::printf("    find failed\n"); return 1; }
    if (got.model != "midashenglm_gen" || got.prompt != "a heavy metal door slam") { std::printf("    fields wrong\n"); rc = 1; }
    if (std::fabs(got.duration_s - 2.0) > 1e-6 || got.seed != 42) { std::printf("    params wrong\n"); rc = 1; }
    if (got.tags.size() != 2 || got.tags[0] != "door") { std::printf("    tags wrong\n"); rc = 1; }
    if (got.source != "user") { std::printf("    source wrong: %s\n", got.source.c_str()); rc = 1; }
    fs::remove_all(user);
    fs::remove_all(shipped);
    return rc;
}

int test_preset_shadowing() {
    namespace fs = std::filesystem;
    std::string user = "/tmp/gaf-preset-shadow-u";
    std::string shipped = "/tmp/gaf-preset-shadow-s";
    fs::remove_all(user);
    fs::remove_all(shipped);
    fs::create_directories(user);
    fs::create_directories(shipped);

    // A shipped preset.
    {
        std::ofstream f(shipped + "/shared.json");
        f << R"({"name":"shared","kind":"sfx","model":"midashenglm_gen","prompt":"shipped version"})";
    }
    // A user preset with the same name shadows it.
    {
        std::ofstream f(user + "/shared.json");
        f << R"({"name":"shared","kind":"sfx","model":"midashenglm_gen","prompt":"user version"})";
    }
    // A shipped-only preset.
    {
        std::ofstream f(shipped + "/only-shipped.json");
        f << R"({"name":"only-shipped","kind":"music","model":"ace_step","prompt":"music"})";
    }

    gaf::Presets store(shipped, user);
    int rc = 0;
    auto all = store.list();
    // Expect: shared (user, shadowing) + only-shipped = 2 entries.
    if (all.size() != 2) { std::printf("    list size=%zu (want 2)\n", all.size()); rc = 1; }
    gaf::Preset shared = store.find("shared");
    if (shared.source != "user" || shared.prompt != "user version") {
        std::printf("    shadowing wrong: src=%s prompt=%s\n", shared.source.c_str(), shared.prompt.c_str());
        rc = 1;
    }
    if (store.find("only-shipped").source != "shipped") { std::printf("    shipped-only wrong\n"); rc = 1; }
    fs::remove_all(user);
    fs::remove_all(shipped);
    return rc;
}

int test_preset_remove() {
    namespace fs = std::filesystem;
    std::string user = "/tmp/gaf-preset-rm";
    fs::remove_all(user);
    fs::create_directories(user);
    std::string shipped = "/tmp/gaf-preset-rm-s";
    fs::remove_all(shipped);
    fs::create_directories(shipped);
    gaf::Presets store(shipped, user);
    std::string err;
    gaf::Preset p;
    p.name = "temp";
    p.kind = "sfx";
    p.model = "midashenglm_gen";
    p.prompt = "temp";
    store.save(p, err);
    int rc = 0;
    if (!store.remove("temp", err)) { std::printf("    remove: %s\n", err.c_str()); rc = 1; }
    if (!store.find("temp").name.empty()) { std::printf("    still findable\n"); rc = 1; }
    // Removing a shipped preset must fail (user-only).
    {
        std::ofstream f(shipped + "/bundled.json");
        f << R"({"name":"bundled","kind":"sfx","model":"midashenglm_gen","prompt":"x"})";
    }
    if (store.remove("bundled", err)) { std::printf("    shipped remove should fail\n"); rc = 1; }
    fs::remove_all(user);
    fs::remove_all(shipped);
    return rc;
}

} // namespace

std::vector<Test> preset_tests() {
    return {
        {"preset_save_find", test_preset_save_find},
        {"preset_shadowing", test_preset_shadowing},
        {"preset_remove", test_preset_remove},
    };
}
