// gaf — Game Audio Forge CLI
//
// Milestone 2 surface:
//   gaf config show            print resolved config + paths
//   gaf models list            list registered models (from the engine)
//   gaf models status          show install/load status
//   gaf generate <model> --text "..." [--duration N] [--seed N] [--out FILE]
//                              run a generation through the engine and save the
//                              WAV + a library record
//
// The CLI is a thin client over the audio.cpp server (see src/gaf/http.cpp).
#include "gaf/config.h"
#include "gaf/http.h"
#include "gaf/library.h"

#include <cstdio>
#include <cstring>
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
        "Usage:\n"
        "  gaf config init            seed user config from the shipped default\n"
        "  gaf config show            print resolved config + paths\n"
        "  gaf models list\n"
        "  gaf models status\n"
        "  gaf generate <model> --text \"prompt\" [--duration N] [--seed N] [--out FILE]\n"
        "  gaf library list [--profile NAME]\n"
        "\n"
        "Environment:\n"
        "  GAF_DATA_DIR / GAF_CONFIG_DIR / GAF_CACHE_DIR  override XDG paths\n"
        "  GAF_SERVER_HOST / GAF_SERVER_PORT              engine server (default 127.0.0.1:17495)\n");
}

struct Opt {
    std::string text;
    double duration = -1.0;
    long seed = -1;
    std::string out;
    std::string profile = "default";
    bool has_duration = false;
    bool has_seed = false;
};

bool parse_opts(int argc, char** argv, int start, Opt& o) {
    for (int i = start; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "missing value for %s\n", name);
                return "";
            }
            return std::string(argv[++i]);
        };
        if (a == "--text") { auto v = next("--text"); if (v.empty()) return false; o.text = v; }
        else if (a == "--duration") { auto v = next("--duration"); if (v.empty()) return false; o.duration = std::stod(v); o.has_duration = true; }
        else if (a == "--seed") { auto v = next("--seed"); if (v.empty()) return false; o.seed = std::stol(v); o.has_seed = true; }
        else if (a == "--out") { auto v = next("--out"); if (v.empty()) return false; o.out = v; }
        else if (a == "--profile") { auto v = next("--profile"); if (v.empty()) return false; o.profile = v; }
        else if (a == "-h" || a == "--help") { usage(); std::exit(0); }
        else { std::fprintf(stderr, "unknown option: %s\n", a.c_str()); return false; }
    }
    return true;
}

gaf::EngineConfig engine_from_env(const gaf::EngineConfig& cfg) {
    gaf::EngineConfig e = cfg;
    if (const char* h = std::getenv("GAF_SERVER_HOST"); h && *h) e.host = h;
    if (const char* p = std::getenv("GAF_SERVER_PORT"); p && *p) e.port = std::atoi(p);
    return e;
}

int cmd_config_init() {
    // Seed the user config from the shipped default (config/gaf.json) if present,
    // otherwise write a minimal default.
    gaf::Paths p = gaf::Paths::resolve();
    std::string user_cfg = p.config_dir + "/gaf.json";
    // Best-effort: create the config dir.
    std::string cmd = "mkdir -p '" + p.config_dir + "'";
    std::system(cmd.c_str());
    // Look for the shipped default next to the binary (../config/gaf.json) or in cwd.
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
    // Try the user config; fall back to defaults if absent.
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
    auto& data = j.at("data");
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

int cmd_generate(const gaf::EngineConfig& e, const std::string& model, const Opt& o) {
    if (o.text.empty()) { std::fprintf(stderr, "--text is required\n"); return 2; }
    json req = {{"model", model}, {"request", json::object()}};
    req["request"]["text"] = o.text;
    if (o.has_duration && o.duration > 0) req["request"]["duration_sec"] = o.duration;
    if (o.has_seed && o.seed >= 0) req["request"]["seed"] = o.seed;

    std::fprintf(stderr, "generating with %s ...\n", model.c_str());
    json res;
    if (!gaf::http_post_json(e.host, e.port, "/v1/tasks/run", req, res, 1800000)) {
        std::fprintf(stderr, "generation request failed\n");
        return 1;
    }
    if (res.contains("error")) {
        std::fprintf(stderr, "engine error: %s\n", res.at("error").dump().c_str());
        return 1;
    }
    json r = res.contains("result") ? res.at("result") : res;

    // Decode base64 audio to a WAV file.
    std::string out_path = o.out;
    if (out_path.empty()) {
        gaf::Paths p = gaf::Paths::resolve();
        out_path = p.libraries_dir() + "/" + o.profile + "/gen_" + model + ".wav";
    }
    // Ensure parent dir.
    {
        std::string dir = out_path;
        auto slash = dir.find_last_of('/');
        if (slash != std::string::npos) {
            dir = dir.substr(0, slash);
            // best-effort mkdir -p
            std::string cmd = "mkdir -p '" + dir + "'";
            std::system(cmd.c_str());
        }
    }

    std::string b64;
    if (r.contains("audio")) b64 = r.at("audio").get<std::string>();
    if (b64.empty()) {
        std::fprintf(stderr, "no audio in result\n");
        return 1;
    }
    // base64 decode
    const std::string tbl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    auto val = [&tbl](char c) -> int {
        auto it = tbl.find(c);
        return it == std::string::npos ? -1 : (int)it;
    };
    std::string raw;
    int bits = 0, acc = 0;
    for (char c : b64) {
        if (c == '=' || c == '\n' || c == '\r') continue;
        int v = val(c);
        if (v < 0) continue;
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            raw.push_back((acc >> bits) & 0xFF);
        }
    }
    {
        std::ofstream f(out_path, std::ios::binary);
        f.write(raw.data(), raw.size());
    }

    // Record in the library.
    gaf::Paths p = gaf::Paths::resolve();
    std::string profile_dir = p.libraries_dir() + "/" + o.profile;
    std::system(("mkdir -p '" + profile_dir + "'").c_str());
    gaf::Library lib(profile_dir);
    std::string err;
    lib.load(err);
    gaf::Take t;
    t.profile = o.profile;
    t.kind = (model.find("midashenglm") != std::string::npos) ? "sfx" : "music";
    t.path = out_path;
    t.prompt = o.text;
    t.model = model;
    t.seed = o.has_seed ? o.seed : 0;
    t.duration_s = r.value("duration", 0.0);
    if (t.duration_s <= 0.0 && r.contains("timing") && r.at("timing").contains("audio_duration_ms"))
        t.duration_s = r.at("timing").at("audio_duration_ms").get<double>() / 1000.0;
    t.sample_rate = r.value("sample_rate", 0);
    t.license = "see model license";
    std::string id = lib.add(t, err);
    if (!id.empty())
        std::fprintf(stderr, "library record: %s\n", id.c_str());

    std::printf("%s\n", out_path.c_str());
    std::fprintf(stderr, "sample_rate=%d channels=%d duration=%.2fs\n",
                 r.value("sample_rate", 0), r.value("channels", 0), t.duration_s);
    if (r.contains("timing"))
        std::fprintf(stderr, "timing: %s\n", r.at("timing").dump().c_str());
    return 0;
}

int cmd_library_list(const gaf::EngineConfig& e, const std::string& profile) {
    (void)e;
    gaf::Paths p = gaf::Paths::resolve();
    std::string profile_dir = p.libraries_dir() + "/" + profile;
    gaf::Library lib(profile_dir);
    std::string err;
    if (!lib.load(err)) { std::fprintf(stderr, "%s\n", err.c_str()); return 1; }
    if (lib.takes().empty()) { std::printf("(library '%s' is empty)\n", profile.c_str()); return 0; }
    for (auto& t : lib.takes())
        std::printf("  %-26s %-6s %-18s %6.2fs  %s\n",
                    t.id.c_str(), t.kind.c_str(), t.model.c_str(), t.duration_s,
                    t.prompt.substr(0, 40).c_str());
    return 0;
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
    if (cmd == "library" && argc >= 3 && std::string(argv[2]) == "list") {
        Opt o;
        if (!parse_opts(argc, argv, 3, o)) return 2;
        return cmd_library_list(e, o.profile);
    }
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
