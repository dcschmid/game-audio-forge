#include "gaf/presets.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace gaf {

namespace {

// Directory containing the running executable (for locating shipped presets).
std::string exe_dir() {
    char buf[4096];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n <= 0) return ".";
    buf[n] = '\0';
    std::filesystem::path p(buf);
    return p.parent_path().string();
}

} // namespace

Presets::Presets(std::string shipped_dir, std::string user_dir)
    : shipped_dir_(std::move(shipped_dir)), user_dir_(std::move(user_dir)) {}

Preset Presets::load_file(const std::string& path, const std::string& source) {
    Preset p;
    p.source = source;
    p.path = path;
    std::ifstream f(path);
    if (!f) return p;
    nlohmann::json j;
    try { f >> j; } catch (...) { return p; }
    if (j.contains("name")) p.name = j.at("name").get<std::string>();
    if (j.contains("kind")) p.kind = j.at("kind").get<std::string>();
    if (j.contains("model")) p.model = j.at("model").get<std::string>();
    if (j.contains("prompt")) p.prompt = j.at("prompt").get<std::string>();
    if (j.contains("duration_s")) p.duration_s = j.at("duration_s").get<double>();
    if (j.contains("seed")) p.seed = j.at("seed").get<long>();
    if (j.contains("tags")) p.tags = j.at("tags").get<std::vector<std::string>>();
    if (j.contains("loop") && j.at("loop").is_object()) {
        p.loop = j.at("loop").value("enabled", false);
        p.crossfade_s = j.at("loop").value("crossfade_s", 0.0);
    }
    if (p.name.empty()) p.name = std::filesystem::path(path).stem().string();
    return p;
}

std::vector<Preset> Presets::list() const {
    auto scan = [&](const std::string& dir, const std::string& source) {
        std::vector<Preset> v;
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) return v;
        for (auto& e : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (!e.is_regular_file()) continue;
            if (e.path().extension().string() != ".json") continue;
            v.push_back(load_file(e.path().string(), source));
        }
        return v;
    };
    std::vector<Preset> user = scan(user_dir_, "user");
    std::vector<Preset> shipped = scan(shipped_dir_, "shipped");
    // User presets shadow shipped ones with the same name.
    std::vector<Preset> out = user;
    for (auto& s : shipped) {
        bool shadowed = false;
        for (auto& u : user)
            if (u.name == s.name) { shadowed = true; break; }
        if (!shadowed) out.push_back(s);
    }
    std::sort(out.begin(), out.end(),
              [](const Preset& a, const Preset& b) { return a.name < b.name; });
    return out;
}

Preset Presets::find(const std::string& name) const {
    // User first (shadows shipped).
    for (const std::string& dir : {user_dir_, shipped_dir_}) {
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec)) continue;
        for (auto& e : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (!e.is_regular_file() || e.path().extension().string() != ".json") continue;
            Preset p = load_file(e.path().string(),
                                 dir == user_dir_ ? "user" : "shipped");
            if (p.name == name) return p;
        }
    }
    return Preset{};
}

bool Presets::save(const Preset& p, std::string& err) const {
    std::error_code ec;
    std::filesystem::create_directories(user_dir_, ec);
    nlohmann::json j;
    j["name"] = p.name;
    j["kind"] = p.kind;
    j["model"] = p.model;
    j["prompt"] = p.prompt;
    j["duration_s"] = p.duration_s;
    j["seed"] = p.seed;
    j["tags"] = p.tags;
    if (p.loop) j["loop"] = {{"enabled", true}, {"crossfade_s", p.crossfade_s}};
    std::string path = user_dir_ + "/" + p.name + ".json";
    std::ofstream f(path);
    if (!f) { err = "cannot write preset: " + p.name; return false; }
    f << j.dump(2) << "\n";
    return true;
}

bool Presets::remove(const std::string& name, std::string& err) const {
    std::string path = user_dir_ + "/" + name + ".json";
    std::error_code ec;
    if (!std::filesystem::remove(path, ec)) {
        err = "user preset not found: " + name;
        return false;
    }
    return true;
}

} // namespace gaf
