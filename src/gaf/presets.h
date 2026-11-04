#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace gaf {

// A reusable generation recipe.
struct Preset {
    std::string name;
    std::string kind;        // sfx | music
    std::string model;       // engine model id, e.g. "midashenglm_gen"
    std::string prompt;
    double duration_s = -1.0;
    long seed = 0;
    std::vector<std::string> tags;
    bool loop = false;
    double crossfade_s = 0.0;
    // Source: "shipped" (repo /presets) or "user" (XDG data dir).
    std::string source;
    std::string path;        // file path (for user presets / delete)
};

// Preset store spanning the shipped (/presets) and user (XDG) locations.
class Presets {
public:
    Presets(std::string shipped_dir, std::string user_dir);

    // All presets (shipped + user), sorted by name.
    std::vector<Preset> list() const;
    // Find by name (user shadows shipped on name collision). nullptr if absent.
    Preset find(const std::string& name) const;
    // Save a user preset (overwrites an existing user preset of the same name).
    bool save(const Preset& p, std::string& err) const;
    // Delete a user preset.
    bool remove(const std::string& name, std::string& err) const;

private:
    std::string shipped_dir_;
    std::string user_dir_;
    static Preset load_file(const std::string& path, const std::string& source);
};

} // namespace gaf
