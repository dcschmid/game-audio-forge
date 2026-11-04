// gaf — Game Audio Forge CLI
//
// Milestone 3 surface:
//   gaf config init|show
//   gaf models list|status
//   gaf profile create|list|show|use|rm
//   gaf preset list|show|save|rm
//   gaf generate <model|preset> [opts]
//   gaf library list|show|rm|import
//
// The CLI is a thin client over the audio.cpp server (see src/gaf/http.cpp).
#include "gaf/config.h"
#include "gaf/http.h"
#include "gaf/library.h"
#include "gaf/presets.h"
#include "gaf/profiles.h"
#include "gaf/qc.h"

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

void usage() {
    std::printf(
        "gaf — Game Audio Forge\n"
        "\n"
        "Config / engine:\n"
        "  gaf config init            seed user config from the shipped default\n"
        "  gaf config show            print resolved config + paths\n"
        "  gaf models list            list models registered with the engine\n"
        "  gaf models status          show model install status\n"
        "\n"
        "Profiles (independent project scopes):\n"
        "  gaf profile create <name> [--desc TEXT] [--rate N] [--bits N]\n"
        "  gaf profile list\n"
        "  gaf profile show [name]\n"
        "  gaf profile use <name>\n"
        "  gaf profile rm <name>\n"
        "\n"
        "Presets (reusable generation recipes):\n"
        "  gaf preset list\n"
        "  gaf preset show <name>\n"
        "  gaf preset save <name> --model M --text P [--kind sfx|music] [--duration N] [--seed N] [--tags a,b]\n"
        "  gaf preset rm <name>\n"
        "\n"
        "Generation:\n"
        "  gaf generate <model|preset> [--text P] [--duration N] [--seed N]\n"
        "       [--count N] [--tags a,b] [--profile NAME] [--out FILE] [--no-qc]\n"
        "\n"
        "Library:\n"
        "  gaf library list [--profile NAME] [--kind K] [--all]\n"
        "  gaf library show <id> [--profile NAME]\n"
        "  gaf library rm <id> [--profile NAME]\n"
        "  gaf library import <file> [--profile NAME] [--kind K] [--prompt P] [--tags a,b]\n"
        "\n"
        "Environment:\n"
        "  GAF_DATA_DIR / GAF_CONFIG_DIR / GAF_CACHE_DIR  override XDG paths\n"
        "  GAF_SERVER_HOST / GAF_SERVER_PORT              engine (default 127.0.0.1:17495)\n");
}

struct Opt {
    std::string text;
    double duration = -1.0;
    long seed = -1;
    std::string out;
    std::string profile;
    int count = 1;
    std::vector<std::string> tags;
    bool has_duration = false;
    bool has_seed = false;
    bool no_qc = false;
    std::string kind;
    std::string desc;
    int rate = 0;
    int bits = 0;
    bool all = false;
    std::string model;
};

bool parse_opts(int argc, char** argv, int start, Opt& o) {
    for (int i = start; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) { std::fprintf(stderr, "missing value for %s\n", name); return ""; }
            return std::string(argv[++i]);
        };
        if (a == "--text" || a == "--prompt") { auto v = next(a.c_str()); if (v.empty()) return false; o.text = v; }
        else if (a == "--duration") { auto v = next("--duration"); if (v.empty()) return false; o.duration = std::stod(v); o.has_duration = true; }
        else if (a == "--seed") { auto v = next("--seed"); if (v.empty()) return false; o.seed = std::stol(v); o.has_seed = true; }
        else if (a == "--out") { auto v = next("--out"); if (v.empty()) return false; o.out = v; }
        else if (a == "--profile") { auto v = next("--profile"); if (v.empty()) return false; o.profile = v; }
        else if (a == "--count") { auto v = next("--count"); if (v.empty()) return false; o.count = std::max(1, std::stoi(v)); }
        else if (a == "--tags") {
            auto v = next("--tags"); if (v.empty()) return false;
            std::stringstream ss(v); std::string t;
            while (std::getline(ss, t, ',')) if (!t.empty()) o.tags.push_back(t);
        }
        else if (a == "--kind") { auto v = next("--kind"); if (v.empty()) return false; o.kind = v; }
        else if (a == "--model") { auto v = next("--model"); if (v.empty()) return false; o.model = v; }
        else if (a == "--desc") { auto v = next("--desc"); if (v.empty()) return false; o.desc = v; }
        else if (a == "--rate") { auto v = next("--rate"); if (v.empty()) return false; o.rate = std::stoi(v); }
        else if (a == "--bits") { auto v = next("--bits"); if (v.empty()) return false; o.bits = std::stoi(v); }
        else if (a == "--no-qc") { o.no_qc = true; }
        else if (a == "--all") { o.all = true; }
        else if (a == "-h" || a == "--help") { usage(); std::exit(0); }
        else if (a.rfind("--", 0) == 0) { std::fprintf(stderr, "unknown option: %s\n", a.c_str()); return false; }
        // else: positional argument — skip (handled by the caller).
    }
    return true;
}

gaf::EngineConfig engine_from_env(const gaf::EngineConfig& cfg) {
    gaf::EngineConfig e = cfg;
    if (const char* h = std::getenv("GAF_SERVER_HOST"); h && *h) e.host = h;
    if (const char* p = std::getenv("GAF_SERVER_PORT"); p && *p) e.port = std::atoi(p);
    return e;
}

// Resolve the active profile name (explicit > state > "default").
std::string resolve_profile(const gaf::Paths& p, const std::string& explicit_name) {
    if (!explicit_name.empty()) return explicit_name;
    gaf::Profiles profs(p.projects_dir(), p.config_dir + "/state.json");
    return profs.active();
}

// base64 decode (engine returns audio as base64 WAV).
std::string b64_decode(const std::string& b64) {
    const std::string tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto val = [&tbl](char c) -> int { auto it = tbl.find(c); return it == std::string::npos ? -1 : (int)it; };
    std::string raw;
    int bits = 0, acc = 0;
    for (char c : b64) {
        if (c == '=' || c == '\n' || c == '\r') continue;
        int v = val(c);
        if (v < 0) continue;
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) { bits -= 8; raw.push_back((acc >> bits) & 0xFF); }
    }
    return raw;
}

void ensure_dir(const std::string& dir) {
    if (dir.empty()) return;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
}

// Run QC on a generated/imported file and fill the take's QC fields.
void apply_qc(gaf::Take& t, const std::string& abs_path) {
    gaf::QcResult q = gaf::analyze_wav(abs_path);
    if (!q.valid) { t.qc_ok = false; return; }
    t.duration_s = q.duration_s;
    t.sample_rate = q.sample_rate;
    t.bit_depth = q.bit_depth;
    t.peak = q.peak;
    t.rms = q.rms;
    t.loudness_lufs = q.loudness_lufs;
    t.silence_head_s = q.silence_head_s;
    t.silence_tail_s = q.silence_tail_s;
    // Flag: silent output, or extreme loudness.
    t.qc_ok = !(q.peak < 0.001) && !(q.loudness_lufs > -3.0);
}

int cmd_config_init() {
    gaf::Paths p = gaf::Paths::resolve();
    std::string user_cfg = p.config_dir + "/gaf.json";
    ensure_dir(p.config_dir);
    std::string shipped;
    for (const char* cand : {"config/gaf.json", "../config/gaf.json"}) {
        std::ifstream f(cand);
        if (f) { shipped = cand; break; }
    }
    std::string content;
    if (!shipped.empty()) {
        std::ifstream f(shipped);
        std::ostringstream ss;
        ss << f.rdbuf();
        content = ss.str();
    } else {
        content = "{\n  \"engine\": {\"host\": \"127.0.0.1\", \"port\": 17495, \"server\": true, "
                  "\"backends\": [\"cuda\", \"vulkan\", \"rocm\", \"cpu\"]},\n  \"models\": {}\n}\n";
    }
    std::ofstream out(user_cfg);
    if (!out) { std::fprintf(stderr, "cannot write %s\n", user_cfg.c_str()); return 1; }
    out << content;
    std::printf("wrote %s\n", user_cfg.c_str());
    return 0;
}

int cmd_config_show() {
    gaf::Config cfg;
    std::string err;
    if (!gaf::Config::load_default(cfg, err)) {
        std::fprintf(stderr, "(no user config at %s — run 'gaf config init')\n",
                     cfg.paths.config_dir.c_str());
        cfg = gaf::Config{};
        cfg.paths = gaf::Paths::resolve();
    }
    std::printf("data_dir:   %s\n", cfg.paths.data_dir.c_str());
    std::printf("config_dir: %s\n", cfg.paths.config_dir.c_str());
    std::printf("cache_dir:  %s\n", cfg.paths.cache_dir.c_str());
    std::printf("models_dir: %s\n", cfg.paths.models_dir().c_str());
    std::printf("engine:     %s  %s:%d  backends=%s\n",
                cfg.engine.name.c_str(), cfg.engine.host.c_str(), cfg.engine.port,
                [&]{ std::string s; for (auto& b : cfg.engine.backends) { if (!s.empty()) s += ","; s += b; } return s; }().c_str());
    std::printf("models:     %zu registered\n", cfg.models.size());
    for (auto& m : cfg.models)
        std::printf("  - %-18s family=%-16s task=%-6s pkg=%s\n",
                    m.id.c_str(), m.family.c_str(), m.task.c_str(), m.package.c_str());
    return 0;
}

int cmd_models_list(const gaf::EngineConfig& e) {
    json j;
    if (!gaf::http_get_json(e.host, e.port, "/v1/models", j)) {
        std::fprintf(stderr, "cannot reach engine at %s:%d\n", e.host.c_str(), e.port);
        return 1;
    }
    auto data = j.value("data", json::array());
    if (data.empty()) { std::printf("(no models configured)\n"); return 0; }
    for (auto& m : data)
        std::printf("  %-18s family=%-16s task=%-6s loaded=%s\n",
                    m.value("id","?").c_str(), m.value("family","?").c_str(),
                    m.value("task","?").c_str(), m.value("loaded",false) ? "yes" : "no");
    return 0;
}

int cmd_models_status(const gaf::EngineConfig& e) {
    json j;
    if (!gaf::http_get_json(e.host, e.port, "/v1/ui/models/install-status", j)) {
        std::fprintf(stderr, "cannot reach engine at %s:%d\n", e.host.c_str(), e.port);
        return 1;
    }
    auto data = j.value("data", json::array());
    if (data.empty()) { std::printf("(no install jobs)\n"); return 0; }
    for (auto& m : data)
        std::printf("  %-28s %-10s %3d%%  %s\n",
                    m.value("id","?").c_str(), m.value("state","?").c_str(),
                    m.value("progress_percent", 0),
                    m.value("message","").c_str());
    return 0;
}

int cmd_profile(const char* sub, int argc, char** argv, const gaf::EngineConfig& e) {
    (void)e;
    gaf::Paths p = gaf::Paths::resolve();
    gaf::Profiles profs(p.projects_dir(), p.config_dir + "/state.json");
    std::string action = sub;

    if (action == "list") {
        auto names = profs.list();
        std::string active = profs.active();
        if (names.empty()) { std::printf("(no profiles)\n"); return 0; }
        for (auto& n : names)
            std::printf("  %-20s %s\n", n.c_str(), n == active ? "(active)" : "");
        return 0;
    }
    if (action == "create") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf profile create <name> [opts]\n"); return 2; }
        Opt o;
        if (!parse_opts(argc, argv, 4, o)) return 2;
        gaf::Profile pr;
        pr.name = argv[3];
        pr.description = o.desc;
        if (o.rate > 0) pr.sample_rate = o.rate;
        if (o.bits > 0) pr.bit_depth = o.bits;
        std::string err;
        if (!profs.create(pr, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("created profile '%s'\n", pr.name.c_str());
        return 0;
    }
    if (action == "show") {
        std::string name = (argc >= 4) ? argv[3] : profs.active();
        gaf::Profile pr;
        if (!profs.get(name, pr)) { std::fprintf(stderr, "profile not found: %s\n", name.c_str()); return 1; }
        std::printf("name:    %s\n", pr.name.c_str());
        std::printf("desc:    %s\n", pr.description.c_str());
        std::printf("rate:    %d\n", pr.sample_rate);
        std::printf("bits:    %d\n", pr.bit_depth);
        std::printf("loudness: %.1f LUFS\n", pr.loudness_target_lufs);
        std::printf("export:  %s / %s\n", pr.export_format.c_str(), pr.export_target.c_str());
        return 0;
    }
    if (action == "use") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf profile use <name>\n"); return 2; }
        std::string err;
        if (!profs.set_active(argv[3], err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("active profile: %s\n", argv[3]);
        return 0;
    }
    if (action == "rm") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf profile rm <name>\n"); return 2; }
        std::string err;
        if (!profs.remove(argv[3], err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("removed profile '%s'\n", argv[3]);
        return 0;
    }
    std::fprintf(stderr, "unknown profile action: %s\n", action.c_str());
    return 2;
}

int cmd_preset(const char* sub, int argc, char** argv, const gaf::EngineConfig& e) {
    (void)e;
    gaf::Paths p = gaf::Paths::resolve();
    // Shipped presets: look next to the binary (../presets) or in cwd.
    std::string shipped;
    for (const char* cand : {"presets", "../presets"}) {
        std::error_code ec;
        if (std::filesystem::exists(cand, ec)) { shipped = cand; break; }
    }
    gaf::Presets store(shipped, p.presets_dir());
    std::string action = sub;

    if (action == "list") {
        auto all = store.list();
        if (all.empty()) { std::printf("(no presets)\n"); return 0; }
        for (auto& pr : all)
            std::printf("  %-24s %-6s %-18s [%s]  %s\n",
                        pr.name.c_str(), pr.kind.c_str(), pr.model.c_str(),
                        pr.source.c_str(), pr.prompt.substr(0, 36).c_str());
        return 0;
    }
    if (action == "show") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf preset show <name>\n"); return 2; }
        gaf::Preset pr = store.find(argv[3]);
        if (pr.name.empty()) { std::fprintf(stderr, "preset not found: %s\n", argv[3]); return 1; }
        std::printf("name:     %s\n", pr.name.c_str());
        std::printf("kind:     %s\n", pr.kind.c_str());
        std::printf("model:    %s\n", pr.model.c_str());
        std::printf("prompt:   %s\n", pr.prompt.c_str());
        std::printf("duration: %.1f\n", pr.duration_s);
        std::printf("seed:     %ld\n", pr.seed);
        std::printf("source:   %s\n", pr.source.c_str());
        std::string tags;
        for (auto& t : pr.tags) { if (!tags.empty()) tags += ","; tags += t; }
        std::printf("tags:     %s\n", tags.c_str());
        return 0;
    }
    if (action == "save") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf preset save <name> --model M --text P [opts]\n"); return 2; }
        Opt o;
        if (!parse_opts(argc, argv, 4, o)) return 2;
        if (o.text.empty()) { std::fprintf(stderr, "--text is required\n"); return 2; }
        if (o.model.empty()) { std::fprintf(stderr, "--model is required\n"); return 2; }
        gaf::Preset pr;
        pr.name = argv[3];
        pr.kind = o.kind.empty() ? "sfx" : o.kind;
        pr.model = o.model;
        pr.prompt = o.text;
        pr.duration_s = o.has_duration ? o.duration : -1.0;
        pr.seed = o.has_seed ? o.seed : 0;
        pr.tags = o.tags;
        std::string err;
        if (!store.save(pr, err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("saved preset '%s' (user)\n", pr.name.c_str());
        return 0;
    }
    if (action == "rm") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf preset rm <name>\n"); return 2; }
        std::string err;
        if (!store.remove(argv[3], err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("removed preset '%s'\n", argv[3]);
        return 0;
    }
    std::fprintf(stderr, "unknown preset action: %s\n", action.c_str());
    return 2;
}

int cmd_library(const char* sub, int argc, char** argv, const gaf::EngineConfig& e) {
    (void)e;
    gaf::Paths p = gaf::Paths::resolve();
    std::string action = sub;
    Opt o;
    if (!parse_opts(argc, argv, 3, o)) return 2;
    std::string profile = resolve_profile(p, o.profile);
    std::string profile_dir = p.libraries_dir() + "/" + profile;
    gaf::Library lib(profile_dir);
    std::string err;
    if (!lib.load(err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }

    if (action == "list") {
        if (lib.takes().empty()) { std::printf("(library '%s' is empty)\n", profile.c_str()); return 0; }
        for (auto& t : lib.takes()) {
            // --kind filters to exactly that kind; otherwise show generated
            // kinds (sfx/music) by default, or everything with --all.
            if (!o.kind.empty()) {
                if (t.kind != o.kind) continue;
            } else if (!o.all && t.kind != "sfx" && t.kind != "music") {
                continue;
            }
            std::printf("  %-26s %-6s %-18s %7.2fs  %6.1f LUFS  %s  %s\n",
                        t.id.c_str(), t.kind.c_str(), t.model.c_str(), t.duration_s,
                        t.loudness_lufs, t.qc_ok ? "ok" : "QC!",
                        t.prompt.substr(0, 32).c_str());
        }
        return 0;
    }
    if (action == "show") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf library show <id>\n"); return 2; }
        const gaf::Take* t = lib.find(argv[3]);
        if (!t) { std::fprintf(stderr, "take not found: %s\n", argv[3]); return 1; }
        std::printf("id:         %s\n", t->id.c_str());
        std::printf("profile:    %s\n", t->profile.c_str());
        std::printf("kind:       %s\n", t->kind.c_str());
        std::printf("path:       %s\n", t->path.c_str());
        std::printf("prompt:     %s\n", t->prompt.c_str());
        std::printf("model:      %s\n", t->model.c_str());
        std::printf("seed:       %ld\n", t->seed);
        std::printf("duration:   %.2fs\n", t->duration_s);
        std::printf("format:     %d Hz / %d-bit\n", t->sample_rate, t->bit_depth);
        std::printf("peak:       %.3f\n", t->peak);
        std::printf("rms:        %.3f\n", t->rms);
        std::printf("loudness:   %.1f LUFS\n", t->loudness_lufs);
        std::printf("silence:    head %.2fs / tail %.2fs\n", t->silence_head_s, t->silence_tail_s);
        std::printf("qc:         %s\n", t->qc_ok ? "ok" : "FLAGGED");
        std::printf("license:    %s\n", t->license.c_str());
        std::printf("created:    %s\n", t->created_at.c_str());
        std::string tags;
        for (auto& tag : t->tags) { if (!tags.empty()) tags += ","; tags += tag; }
        std::printf("tags:       %s\n", tags.c_str());
        return 0;
    }
    if (action == "rm") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf library rm <id>\n"); return 2; }
        if (!lib.remove(argv[3], err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("removed take %s\n", argv[3]);
        return 0;
    }
    if (action == "import") {
        if (argc < 4) { std::fprintf(stderr, "usage: gaf library import <file> [opts]\n"); return 2; }
        std::string src = argv[3];
        std::error_code ec;
        if (!std::filesystem::exists(src, ec)) { std::fprintf(stderr, "file not found: %s\n", src.c_str()); return 1; }
        ensure_dir(profile_dir);
        std::string dest = profile_dir + "/" + std::filesystem::path(src).filename().string();
        std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) { std::fprintf(stderr, "copy failed: %s\n", ec.message().c_str()); return 1; }
        gaf::Take t;
        t.profile = profile;
        t.kind = o.kind.empty() ? "import" : o.kind;
        t.path = std::filesystem::path(src).filename().string();
        t.prompt = o.text;
        t.model = "import";
        t.tags = o.tags;
        t.license = "imported";
        if (!o.no_qc) apply_qc(t, dest);
        std::string id = lib.add(t, err);
        if (id.empty()) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
        std::printf("imported %s as %s\n", src.c_str(), id.c_str());
        return 0;
    }
    std::fprintf(stderr, "unknown library action: %s\n", action.c_str());
    return 2;
}

int cmd_generate(const gaf::EngineConfig& e, const std::string& target, const Opt& o) {
    gaf::Paths p = gaf::Paths::resolve();
    std::string profile = resolve_profile(p, o.profile);

    // Resolve target: a preset name or an engine model id.
    std::string shipped;
    for (const char* cand : {"presets", "../presets"}) {
        std::error_code ec;
        if (std::filesystem::exists(cand, ec)) { shipped = cand; break; }
    }
    gaf::Presets store(shipped, p.presets_dir());
    std::string model = target;
    std::string prompt = o.text;
    double duration = o.has_duration ? o.duration : -1.0;
    long seed = o.has_seed ? o.seed : -1;
    std::vector<std::string> tags = o.tags;
    std::string kind;
    gaf::Preset pr = store.find(target);
    if (!pr.name.empty()) {
        model = pr.model;
        if (prompt.empty()) prompt = pr.prompt;
        if (duration < 0) duration = pr.duration_s;
        if (seed < 0) seed = pr.seed;
        if (tags.empty()) tags = pr.tags;
        kind = pr.kind;
    }
    if (prompt.empty()) { std::fprintf(stderr, "--text is required (or use a preset)\n"); return 2; }
    if (kind.empty()) kind = (model.find("midashenglm") != std::string::npos) ? "sfx" : "music";

    std::string profile_dir = p.libraries_dir() + "/" + profile;
    ensure_dir(profile_dir);
    gaf::Library lib(profile_dir);
    std::string err;
    lib.load(err);

    int rc = 0;
    for (int n = 0; n < o.count; ++n) {
        json req = {{"model", model}, {"request", json::object()}};
        req["request"]["text"] = prompt;
        if (duration > 0) req["request"]["duration_sec"] = duration;
        if (seed >= 0) req["request"]["seed"] = seed;
        if (n > 0 && seed >= 0) req["request"]["seed"] = seed + n; // vary retakes

        std::fprintf(stderr, "[%d/%d] generating with %s ...\n", n + 1, o.count, model.c_str());
        json res;
        if (!gaf::http_post_json(e.host, e.port, "/v1/tasks/run", req, res, 1800000)) {
            std::fprintf(stderr, "generation request failed\n");
            rc = 1;
            continue;
        }
        if (res.contains("error")) {
            std::fprintf(stderr, "engine error: %s\n", res.at("error").dump().c_str());
            rc = 1;
            continue;
        }
        json r = res.contains("result") ? res.at("result") : res;

        std::string b64;
        if (r.contains("audio")) b64 = r.at("audio").get<std::string>();
        if (b64.empty()) { std::fprintf(stderr, "no audio in result\n"); rc = 1; continue; }
        std::string raw = b64_decode(b64);

        std::string out_path = o.out;
        std::string rel_path;
        if (out_path.empty()) {
            rel_path = kind + "/" + model + "_" + std::to_string(n + 1) + ".wav";
            out_path = profile_dir + "/" + rel_path;
        } else {
            rel_path = std::filesystem::path(out_path).filename().string();
        }
        ensure_dir(std::filesystem::path(out_path).parent_path().string());
        {
            std::ofstream f(out_path, std::ios::binary);
            f.write(raw.data(), raw.size());
        }

        gaf::Take t;
        t.profile = profile;
        t.kind = kind;
        t.path = rel_path;
        t.prompt = prompt;
        t.model = model;
        t.seed = seed >= 0 ? (seed + n) : 0;
        t.tags = tags;
        t.license = "see model license";
        if (!o.no_qc) apply_qc(t, out_path);
        std::string id = lib.add(t, err);
        if (id.empty()) { std::fprintf(stderr, "library add failed: %s\n", err.c_str()); rc = 1; continue; }

        std::printf("%s\n", out_path.c_str());
        std::fprintf(stderr, "  id=%s %dHz/%d-bit %.2fs peak=%.3f %.1f LUFS %s\n",
                     id.c_str(), t.sample_rate, t.bit_depth, t.duration_s, t.peak,
                     t.loudness_lufs, t.qc_ok ? "qc-ok" : "QC-FLAGGED");
        if (r.contains("timing"))
            std::fprintf(stderr, "  timing: %s\n", r.at("timing").dump().c_str());
    }
    return rc;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) { usage(); return 2; }
    std::string cmd = argv[1];

    gaf::Config cfg;
    std::string err;
    if (!gaf::Config::load_default(cfg, err)) cfg = gaf::Config{};
    cfg.paths = gaf::Paths::resolve();
    gaf::EngineConfig e = engine_from_env(cfg.engine);

    if (cmd == "config" && argc >= 3 && std::string(argv[2]) == "show")
        return cmd_config_show();
    if (cmd == "config" && argc >= 3 && std::string(argv[2]) == "init")
        return cmd_config_init();
    if (cmd == "models" && argc >= 3 && std::string(argv[2]) == "list")
        return cmd_models_list(e);
    if (cmd == "models" && argc >= 3 && std::string(argv[2]) == "status")
        return cmd_models_status(e);
    if (cmd == "profile" && argc >= 3)
        return cmd_profile(argv[2], argc, argv, e);
    if (cmd == "preset" && argc >= 3)
        return cmd_preset(argv[2], argc, argv, e);
    if (cmd == "library" && argc >= 3)
        return cmd_library(argv[2], argc, argv, e);
    if (cmd == "generate" && argc >= 3) {
        std::string model = argv[2];
        Opt o;
        if (!parse_opts(argc, argv, 3, o)) return 2;
        return cmd_generate(e, model, o);
    }
    if (cmd == "-h" || cmd == "--help" || cmd == "help") { usage(); return 0; }
    std::fprintf(stderr, "unknown command: %s\n", cmd.c_str());
    usage();
    return 2;
}
