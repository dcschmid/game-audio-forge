#include "gaf/profiles.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace gaf {

Profiles::Profiles(std::string projects_dir, std::string state_file)
    : projects_dir_(std::move(projects_dir)), state_file_(std::move(state_file)) {}

std::vector<std::string> Profiles::list() const {
    std::vector<std::string> names;
    std::error_code ec;
    if (!std::filesystem::exists(projects_dir_, ec)) return names;
    for (auto& e : std::filesystem::directory_iterator(projects_dir_, ec)) {
        if (!e.is_regular_file()) continue;
        auto p = e.path().filename().string();
        if (p.size() > 5 && p.substr(p.size() - 5) == ".json")
            names.push_back(p.substr(0, p.size() - 5));
    }
    std::sort(names.begin(), names.end());
    return names;
}

bool Profiles::get(const std::string& name, Profile& out) const {
    std::ifstream f(state_path(name));
    if (!f) return false;
    nlohmann::json j;
    try { f >> j; } catch (...) { return false; }
    out.name = name;
    if (j.contains("description")) out.description = j.at("description").get<std::string>();
    if (j.contains("defaults")) {
        const auto& d = j.at("defaults");
        if (d.contains("sample_rate")) out.sample_rate = d.at("sample_rate").get<int>();
        if (d.contains("bit_depth")) out.bit_depth = d.at("bit_depth").get<int>();
        if (d.contains("loudness_target_lufs")) out.loudness_target_lufs = d.at("loudness_target_lufs").get<double>();
    }
    if (j.contains("export")) {
        const auto& e = j.at("export");
        if (e.contains("format")) out.export_format = e.at("format").get<std::string>();
        if (e.contains("target")) out.export_target = e.at("target").get<std::string>();
    }
    return true;
}

bool Profiles::create(const Profile& p, std::string& err) const {
    std::error_code ec;
    std::filesystem::create_directories(projects_dir_, ec);
    if (std::filesystem::exists(state_path(p.name))) {
        err = "profile already exists: " + p.name;
        return false;
    }
    nlohmann::json j;
    j["name"] = p.name;
    j["description"] = p.description;
    j["defaults"] = {
        {"sample_rate", p.sample_rate},
        {"bit_depth", p.bit_depth},
        {"loudness_target_lufs", p.loudness_target_lufs},
    };
    j["export"] = {
        {"format", p.export_format},
        {"target", p.export_target},
    };
    std::ofstream f(state_path(p.name));
    if (!f) { err = "cannot write profile: " + p.name; return false; }
    f << j.dump(2) << "\n";
    return true;
}

bool Profiles::remove(const std::string& name, std::string& err) const {
    std::error_code ec;
    if (!std::filesystem::remove(state_path(name), ec)) {
        err = "profile not found: " + name;
        return false;
    }
    return true;
}

std::string Profiles::active() const {
    std::ifstream f(state_file_);
    if (!f) return "default";
    nlohmann::json j;
    try { f >> j; } catch (...) { return "default"; }
    if (j.contains("active_profile")) return j.at("active_profile").get<std::string>();
    return "default";
}

bool Profiles::set_active(const std::string& name, std::string& err) const {
    nlohmann::json j;
    {
        std::ifstream f(state_file_);
        if (f) { try { f >> j; } catch (...) { j = nlohmann::json::object(); } }
    }
    if (!j.is_object()) j = nlohmann::json::object();
    j["active_profile"] = name;
    // Ensure the state file's parent dir exists.
    std::string dir = state_file_.substr(0, state_file_.find_last_of('/'));
    if (!dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
    }
    std::ofstream f(state_file_);
    if (!f) { err = "cannot write state: " + state_file_; return false; }
    f << j.dump(2) << "\n";
    return true;
}

} // namespace gaf
