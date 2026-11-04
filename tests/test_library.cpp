#include "test_main.h"
#include "gaf/library.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace {

// add() must persist a take and assign an id + timestamp.
int test_library_add_and_find() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-lib-test";
    fs::remove_all(dir);
    fs::create_directories(dir);

    gaf::Library lib(dir);
    std::string err;
    if (!lib.load(err)) { std::printf("    load: %s\n", err.c_str()); return 1; }

    gaf::Take t;
    t.profile = "default";
    t.kind = "sfx";
    t.path = "sfx/door.wav";
    t.prompt = "door slam";
    t.model = "midashenglm_gen";
    t.seed = 42;
    t.duration_s = 3.0;
    t.sample_rate = 16000;
    t.license = "Apache-2.0";
    t.tags = {"door", "impact"};

    std::string id = lib.add(t, err);
    int rc = 0;
    if (id.empty()) { std::printf("    add failed: %s\n", err.c_str()); return 1; }
    if (id.size() != 26) { std::printf("    id len=%zu\n", id.size()); rc = 1; }

    const gaf::Take* found = lib.find(id);
    if (!found) { std::printf("    find returned null\n"); rc = 1; }
    else {
        if (found->prompt != "door slam") { std::printf("    prompt=%s\n", found->prompt.c_str()); rc = 1; }
        if (found->seed != 42) { std::printf("    seed=%ld\n", found->seed); rc = 1; }
        if (found->created_at.empty()) { std::printf("    created_at empty\n"); rc = 1; }
        if (found->tags.size() != 2 || found->tags[0] != "door") { std::printf("    tags wrong\n"); rc = 1; }
    }
    if (lib.find("nonexistent") != nullptr) { std::printf("    find(nonexistent) should be null\n"); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

// load() must round-trip a saved library from disk.
int test_library_roundtrip() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-lib-rt";
    fs::remove_all(dir);
    fs::create_directories(dir);

    {
        gaf::Library lib(dir);
        std::string err;
        lib.load(err);
        gaf::Take t;
        t.profile = "demo";
        t.kind = "music";
        t.path = "music/loop.wav";
        t.prompt = "cave ambience loop";
        t.model = "ace_step";
        t.duration_s = 24.0;
        t.sample_rate = 48000;
        t.bit_depth = 16;
        lib.add(t, err);
    }

    // Reload in a fresh instance.
    gaf::Library lib2(dir);
    std::string err;
    if (!lib2.load(err)) { std::printf("    load: %s\n", err.c_str()); return 1; }
    int rc = 0;
    if (lib2.takes().size() != 1) { std::printf("    takes=%zu\n", lib2.takes().size()); rc = 1; }
    else {
        auto& t = lib2.takes()[0];
        if (t.kind != "music" || t.model != "ace_step" || t.duration_s != 24.0 || t.sample_rate != 48000) {
            std::printf("    roundtrip fields wrong\n"); rc = 1;
        }
        if (t.id.empty()) { std::printf("    id lost in roundtrip\n"); rc = 1; }
    }
    fs::remove_all(dir);
    return rc;
}

// load() on a missing library file must succeed with zero takes.
int test_library_empty() {
    namespace fs = std::filesystem;
    std::string dir = "/tmp/gaf-lib-empty";
    fs::remove_all(dir);
    fs::create_directories(dir);
    gaf::Library lib(dir);
    std::string err;
    int rc = 0;
    if (!lib.load(err)) { std::printf("    load: %s\n", err.c_str()); rc = 1; }
    if (!lib.takes().empty()) { std::printf("    takes=%zu\n", lib.takes().size()); rc = 1; }
    fs::remove_all(dir);
    return rc;
}

} // namespace

std::vector<Test> library_tests() {
    return {
        {"library_add_and_find", test_library_add_and_find},
        {"library_roundtrip", test_library_roundtrip},
        {"library_empty", test_library_empty},
    };
}
