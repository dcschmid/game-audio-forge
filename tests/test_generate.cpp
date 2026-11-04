#include "test_main.h"
#include "gaf/http.h"
#include "gaf/qc.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

// Reachable engine? (the server exposes /v1/models, not /v1/health)
bool engine_up(const std::string& host, int port) {
    nlohmann::json j;
    return gaf::http_get_json(host, port, "/v1/models", j, 3000);
}

std::string env_or(const char* k, const std::string& d) {
    const char* v = std::getenv(k);
    return (v && *v) ? v : d;
}

// base64 decode (mirrors the CLI helper).
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

// Full round-trip: POST /v1/tasks/run for a short SFX, decode the base64 WAV,
// verify it parses and is non-silent. Skips (returns 0) when no engine is up.
int test_generate_roundtrip() {
    std::string host = env_or("GAF_SERVER_HOST", "127.0.0.1");
    int port = std::atoi(env_or("GAF_SERVER_PORT", "17495").c_str());
    if (!engine_up(host, port)) {
        std::printf("    (skipped: no engine at %s:%d)\n", host.c_str(), port);
        return 0;
    }

    nlohmann::json req = {
        {"model", "midashenglm_gen"},
        {"request", {{"text", "a short sharp laser zap"},
                     {"duration_sec", 2.0},
                     {"seed", 7}}},
    };
    nlohmann::json res;
    if (!gaf::http_post_json(host, port, "/v1/tasks/run", req, res, 1800000)) {
        std::printf("    request failed\n");
        return 1;
    }
    if (res.contains("error")) {
        std::printf("    engine error: %s\n", res.at("error").dump().c_str());
        return 1;
    }
    nlohmann::json r = res.contains("result") ? res.at("result") : res;
    std::string b64;
    if (r.contains("audio")) b64 = r.at("audio").get<std::string>();
    if (b64.empty()) { std::printf("    no audio in result\n"); return 1; }

    std::string raw = b64_decode(b64);
    std::string path = "/tmp/gaf-roundtrip.wav";
    {
        std::ofstream f(path, std::ios::binary);
        f.write(raw.data(), raw.size());
    }
    gaf::QcResult q = gaf::analyze_wav(path);
    std::filesystem::remove(path);
    int rc = 0;
    if (!q.valid) { std::printf("    wav invalid: %s\n", q.error.c_str()); return 1; }
    if (q.peak < 0.001) { std::printf("    output is silent (peak=%.4f)\n", q.peak); rc = 1; }
    if (q.duration_s <= 0.0) { std::printf("    duration=%.2f\n", q.duration_s); rc = 1; }
    std::printf("    ok: %dHz/%d-bit %.2fs peak=%.3f\n",
                q.sample_rate, q.bit_depth, q.duration_s, q.peak);
    return rc;
}

} // namespace

std::vector<Test> generate_tests() {
    return {
        {"generate_roundtrip", test_generate_roundtrip},
    };
}
