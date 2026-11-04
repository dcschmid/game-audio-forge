#include "test_main.h"
#include "gaf/profiles.h"

#include <cstdio>
#include <filesystem>

namespace {

int test_profile_create_get() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-prof-test";
    fs::remove_all(dir);
    fs::create_directories(dir);
    std::string state = dir + "/state.json";

    gaf::Profiles profs(dir, state);
    std::string err;
    gaf::Profile p;
    p.name = "demo";
    p.description = "a demo game";
    p.sample_rate = 48000;
    p.bit_depth = 16;
    p.loudness_target_lufs = -18.0;
    p.export_target = "defold";
    if (!profs.create(p, err)) { std::printf("    create: %s\n", err.c_str()); return 1; }

    // Duplicate create must fail.
    if (profs.create(p, err)) { std::printf("    duplicate create should fail\n"); return 1; }

    gaf::Profile got;
    if (!profs.get("demo", got)) { std::printf("    get failed\n"); return 1; }
    int rc = 0;
    if (got.name != "demo" || got.description != "a demo game") { std::printf("    fields wrong\n"); rc = 1; }
    if (got.sample_rate != 48000 || got.bit_depth != 16) { std::printf("    defaults wrong\n"); rc = 1; }
    if (std::fabs(got.loudness_target_lufs - (-18.0)) > 1e-6) { std::printf("    lufs wrong\n"); rc = 1; }
    if (got.export_target != "defold") { std::printf("    export wrong\n"); rc = 1; }
    if (profs.get("nope", got)) { std::printf("    get(nope) should fail\n"); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

int test_profile_list_remove() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-prof-lr";
    fs::remove_all(dir);
    fs::create_directories(dir);
    std::string state = dir + "/state.json";
    gaf::Profiles profs(dir, state);
    std::string err;
    gaf::Profile a; a.name = "alpha";
    gaf::Profile b; b.name = "beta";
    profs.create(a, err);
    profs.create(b, err);
    int rc = 0;
    auto names = profs.list();
    if (names.size() != 2 || names[0] != "alpha" || names[1] != "beta") {
        std::printf("    list wrong (%zu)\n", names.size()); rc = 1;
    }
    if (!profs.remove("alpha", err)) { std::printf("    remove failed\n"); rc = 1; }
    if (profs.remove("alpha", err)) { std::printf("    double remove should fail\n"); rc = 1; }
    if (profs.list().size() != 1) { std::printf("    list after remove wrong\n"); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

int test_profile_active() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-prof-act";
    fs::remove_all(dir);
    fs::create_directories(dir);
    std::string state = dir + "/state.json";
    gaf::Profiles profs(dir, state);
    int rc = 0;
    if (profs.active() != "default") { std::printf("    default active wrong\n"); rc = 1; }
    std::string err;
    if (!profs.set_active("mygame", err)) { std::printf("    set_active: %s\n", err.c_str()); rc = 1; }
    if (profs.active() != "mygame") { std::printf("    active not persisted\n"); rc = 1; }
    // A fresh instance must read the same active profile.
    gaf::Profiles profs2(dir, state);
    if (profs2.active() != "mygame") { std::printf("    active not reloaded\n"); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

} // namespace

std::vector<Test> profile_tests() {
    return {
        {"profile_create_get", test_profile_create_get},
        {"profile_list_remove", test_profile_list_remove},
        {"profile_active", test_profile_active},
    };
}
