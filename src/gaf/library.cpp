#include "gaf/library.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <random>
#include <sstream>

namespace gaf {

namespace {

std::string now_iso8601() {
    auto t = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(t);
    std::tm tm{};
    gmtime_r(&tt, &tm);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm);
    return buf;
}

std::string new_id() {
    // 26-char Crockford-style ULID (time + random). Good enough for local ids.
    static const char* alphabet = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";
    std::random_device rd;
    std::mt19937_64 gen(rd());
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    std::string id;
    id.reserve(26);
    // 10 chars of time (48-bit ms), 16 chars of random.
    unsigned long long t = static_cast<unsigned long long>(now_ms);
    for (int i = 0; i < 10; ++i) { id.push_back(alphabet[t & 0x1F]); t >>= 5; }
    for (int i = 0; i < 16; ++i) {
        id.push_back(alphabet[gen() & 0x1F]);
    }
    return id;
}

} // namespace

Library::Library(std::string profile_dir) : profile_dir_(std::move(profile_dir)) {}

nlohmann::json Library::to_json(const Take& t) {
    return nlohmann::json{
        {"id", t.id},
        {"profile", t.profile},
        {"kind", t.kind},
        {"path", t.path},
        {"prompt", t.prompt},
        {"model", t.model},
        {"seed", t.seed},
        {"duration_s", t.duration_s},
        {"sample_rate", t.sample_rate},
        {"bit_depth", t.bit_depth},
        {"license", t.license},
        {"tags", t.tags},
        {"created_at", t.created_at},
        {"peak", t.peak},
        {"rms", t.rms},
        {"loudness_lufs", t.loudness_lufs},
        {"silence_head_s", t.silence_head_s},
        {"silence_tail_s", t.silence_tail_s},
        {"qc_ok", t.qc_ok},
    };
}

Take Library::from_json(const nlohmann::json& j) {
    Take t;
    if (j.contains("id")) t.id = j.at("id").get<std::string>();
    if (j.contains("profile")) t.profile = j.at("profile").get<std::string>();
    if (j.contains("kind")) t.kind = j.at("kind").get<std::string>();
    if (j.contains("path")) t.path = j.at("path").get<std::string>();
    if (j.contains("prompt")) t.prompt = j.at("prompt").get<std::string>();
    if (j.contains("model")) t.model = j.at("model").get<std::string>();
    if (j.contains("seed")) t.seed = j.at("seed").get<long>();
    if (j.contains("duration_s")) t.duration_s = j.at("duration_s").get<double>();
    if (j.contains("sample_rate")) t.sample_rate = j.at("sample_rate").get<int>();
    if (j.contains("bit_depth")) t.bit_depth = j.at("bit_depth").get<int>();
    if (j.contains("license")) t.license = j.at("license").get<std::string>();
    if (j.contains("tags")) t.tags = j.at("tags").get<std::vector<std::string>>();
    if (j.contains("created_at")) t.created_at = j.at("created_at").get<std::string>();
    if (j.contains("peak")) t.peak = j.at("peak").get<double>();
    if (j.contains("rms")) t.rms = j.at("rms").get<double>();
    if (j.contains("loudness_lufs")) t.loudness_lufs = j.at("loudness_lufs").get<double>();
    if (j.contains("silence_head_s")) t.silence_head_s = j.at("silence_head_s").get<double>();
    if (j.contains("silence_tail_s")) t.silence_tail_s = j.at("silence_tail_s").get<double>();
    if (j.contains("qc_ok")) t.qc_ok = j.at("qc_ok").get<bool>();
    return t;
}

bool Library::load(std::string& err) {
    takes_.clear();
    std::string file = profile_dir_ + "/library.json";
    std::ifstream f(file);
    if (!f) return true; // no library yet
    nlohmann::json j;
    try {
        f >> j;
    } catch (const std::exception& e) {
        err = "library parse error: " + std::string(e.what());
        return false;
    }
    if (j.contains("takes") && j.at("takes").is_array())
        for (auto& t : j.at("takes")) takes_.push_back(from_json(t));
    return true;
}

bool Library::save(std::string& err) const {
    nlohmann::json j;
    j["profile"] = profile_dir_;
    j["takes"] = nlohmann::json::array();
    for (auto& t : takes_) j["takes"].push_back(to_json(t));
    std::ofstream f(profile_dir_ + "/library.json");
    if (!f) { err = "cannot write library: " + profile_dir_ + "/library.json"; return false; }
    f << j.dump(2) << "\n";
    return true;
}

std::string Library::add(const Take& take_in, std::string& err) {
    Take t = take_in;
    if (t.id.empty()) t.id = new_id();
    if (t.created_at.empty()) t.created_at = now_iso8601();
    takes_.push_back(t);
    if (!save(err)) return "";
    return t.id;
}

bool Library::remove(const std::string& id, std::string& err) {
    auto it = std::find_if(takes_.begin(), takes_.end(),
                           [&](const Take& t) { return t.id == id; });
    if (it == takes_.end()) {
        err = "take not found: " + id;
        return false;
    }
    takes_.erase(it);
    return save(err);
}

const Take* Library::find(const std::string& id) const {
    for (auto& t : takes_)
        if (t.id == id) return &t;
    return nullptr;
}

} // namespace gaf
