#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace gaf {

// A project profile: an independent library scope + settings.
//
// Profiles live in the XDG data dir under projects/<name>.json. The active
// profile is stored in the config dir (state.json) so `gaf generate` knows
// which library to write to by default.
struct Profile {
    std::string name;
    std::string description;
    int sample_rate = 48000;
    int bit_depth = 16;
    double loudness_target_lufs = -16.0;
    std::string export_format = "wav";
    std::string export_target = "defold";
};

class Profiles {
public:
    explicit Profiles(std::string projects_dir, std::string state_file);

    // List profile names (sorted).
    std::vector<std::string> list() const;
    // Load a profile by name. Returns false if it does not exist.
    bool get(const std::string& name, Profile& out) const;
    // Create a profile (fails if it already exists).
    bool create(const Profile& p, std::string& err) const;
    // Delete a profile (its library dir is left in place).
    bool remove(const std::string& name, std::string& err) const;

    // Active profile (default "default").
    std::string active() const;
    bool set_active(const std::string& name, std::string& err) const;

    const std::string& projects_dir() const { return projects_dir_; }

private:
    std::string projects_dir_;
    std::string state_file_;
    std::string state_path(const std::string& name) const {
        return projects_dir_ + "/" + name + ".json";
    }
};

} // namespace gaf
