#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace gaf {

// A single take in a profile library.
struct Take {
    std::string id;            // stable id (ULID-like)
    std::string profile;
    std::string kind;          // sfx | music | loop | import
    std::string path;          // audio file path (relative to profile dir)
    std::string prompt;
    std::string model;         // model family + package
    long seed = 0;
    double duration_s = 0.0;
    int sample_rate = 0;
    int bit_depth = 0;
    std::string license;
    std::vector<std::string> tags;
    std::string created_at;    // ISO-8601
    // Quality-control results (filled at generation/import time).
    double peak = 0.0;
    double rms = 0.0;
    double loudness_lufs = 0.0;
    double silence_head_s = 0.0;
    double silence_tail_s = 0.0;
    bool qc_ok = true;         // false if QC flagged a problem
};

// A profile library backed by a JSON file (library.json) in the profile dir.
//
// JSON (not SQLite) keeps M2 dependency-free; the on-disk shape matches the
// library record contract in docs/DATA-LAYOUT.md and can be migrated to
// SQLite later without changing the API.
class Library {
public:
    explicit Library(std::string profile_dir);

    // Load from disk (no-op if the file does not exist yet).
    bool load(std::string& err);
    // Persist to disk.
    bool save(std::string& err) const;

    const std::string& profile_dir() const { return profile_dir_; }
    const std::vector<Take>& takes() const { return takes_; }

    // Add a take and persist. Returns the take id.
    std::string add(const Take& take, std::string& err);
    // Remove a take by id (the audio file is left in place). Returns false if
    // the id is not found.
    bool remove(const std::string& id, std::string& err);
    // Find a take by id (nullptr if absent).
    const Take* find(const std::string& id) const;

private:
    std::string profile_dir_;
    std::vector<Take> takes_;

    static nlohmann::json to_json(const Take& t);
    static Take from_json(const nlohmann::json& j);
};

} // namespace gaf
