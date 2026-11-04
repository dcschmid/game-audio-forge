#include "test_main.h"
#include "gaf/qc.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

// Write a minimal 16-bit PCM WAV.
void write_wav(const std::string& path, int sample_rate, int channels,
               const std::vector<int16_t>& samples) {
    uint32_t data_bytes = static_cast<uint32_t>(samples.size() * 2);
    uint16_t fmt = 1, bits = 16;
    uint32_t byte_rate = sample_rate * channels * 2;
    uint16_t block_align = channels * 2;
    std::ofstream f(path, std::ios::binary);
    auto w32 = [&](uint32_t v) { char b[4] = {char(v), char(v >> 8), char(v >> 16), char(v >> 24)}; f.write(b, 4); };
    auto w16 = [&](uint16_t v) { char b[2] = {char(v), char(v >> 8)}; f.write(b, 2); };
    f.write("RIFF", 4); w32(36 + data_bytes); f.write("WAVE", 4);
    f.write("fmt ", 4); w32(16); w16(fmt); w16(channels); w32(sample_rate);
    w32(byte_rate); w16(block_align); w16(bits);
    f.write("data", 4); w32(data_bytes);
    for (auto s : samples) w16(static_cast<uint16_t>(s));
}

// A 1-second 440 Hz sine at 8 kHz, mono, 16-bit, full-ish scale.
std::vector<int16_t> sine(int sample_rate, int seconds, double hz, int16_t amp) {
    std::vector<int16_t> v;
    int n = sample_rate * seconds;
    for (int i = 0; i < n; ++i)
        v.push_back(static_cast<int16_t>(amp * std::sin(2.0 * M_PI * hz * i / sample_rate)));
    return v;
}

int test_qc_sine() {
    std::string path = "/tmp/gaf-qc-sine.wav";
    write_wav(path, 8000, 1, sine(8000, 1, 440, 16000));
    gaf::QcResult r = gaf::analyze_wav(path);
    int rc = 0;
    if (!r.valid) { std::printf("    invalid: %s\n", r.error.c_str()); return 1; }
    if (r.sample_rate != 8000 || r.channels != 1 || r.bit_depth != 16) { std::printf("    fmt wrong\n"); rc = 1; }
    if (std::fabs(r.duration_s - 1.0) > 0.01) { std::printf("    duration=%.3f\n", r.duration_s); rc = 1; }
    // 440 Hz sine at amp 16000/32768 = 0.488: peak ~0.488, RMS ~0.345.
    if (std::fabs(r.peak - 0.488) > 0.02) { std::printf("    peak=%.3f\n", r.peak); rc = 1; }
    if (std::fabs(r.rms - 0.345) > 0.03) { std::printf("    rms=%.3f\n", r.rms); rc = 1; }
    if (r.silence_head_s > 0.05 || r.silence_tail_s > 0.05) { std::printf("    silence head/tail too big\n"); rc = 1; }
    // Loudness: sine RMS -7 dBFS -> ~ -4 LUFS by our approximation.
    if (r.loudness_lufs > -1.0 || r.loudness_lufs < -8.0) { std::printf("    lufs=%.1f\n", r.loudness_lufs); rc = 1; }
    std::filesystem::remove(path);
    return rc;
}

int test_qc_silence() {
    std::string path = "/tmp/gaf-qc-sil.wav";
    std::vector<int16_t> v(8000, 0); // 1 s of digital silence
    write_wav(path, 8000, 1, v);
    gaf::QcResult r = gaf::analyze_wav(path);
    int rc = 0;
    if (!r.valid) { std::printf("    invalid: %s\n", r.error.c_str()); return 1; }
    if (r.peak > 0.0) { std::printf("    peak=%.4f\n", r.peak); rc = 1; }
    if (r.silence_head_s < 0.99) { std::printf("    silence_head=%.3f\n", r.silence_head_s); rc = 1; }
    std::filesystem::remove(path);
    return rc;
}

int test_qc_head_tail_silence() {
    std::string path = "/tmp/gaf-qc-ht.wav";
    // 0.25 s silence + 0.5 s tone + 0.25 s silence at 8 kHz.
    std::vector<int16_t> v;
    v.insert(v.end(), 2000, 0);
    auto tone = sine(8000, 1, 440, 16000);
    v.insert(v.end(), tone.begin() + 2000, tone.begin() + 6000);
    v.insert(v.end(), 2000, 0);
    write_wav(path, 8000, 1, v);
    gaf::QcResult r = gaf::analyze_wav(path);
    int rc = 0;
    if (!r.valid) { std::printf("    invalid: %s\n", r.error.c_str()); return 1; }
    if (std::fabs(r.silence_head_s - 0.25) > 0.02) { std::printf("    head=%.3f\n", r.silence_head_s); rc = 1; }
    if (std::fabs(r.silence_tail_s - 0.25) > 0.02) { std::printf("    tail=%.3f\n", r.silence_tail_s); rc = 1; }
    std::filesystem::remove(path);
    return rc;
}

int test_qc_invalid() {
    std::string path = "/tmp/gaf-qc-bad.wav";
    { std::ofstream f(path, std::ios::binary); f << "not a wav file at all"; }
    gaf::QcResult r = gaf::analyze_wav(path);
    int rc = 0;
    if (r.valid) { std::printf("    should be invalid\n"); rc = 1; }
    if (r.error.empty()) { std::printf("    expected error\n"); rc = 1; }
    std::filesystem::remove(path);
    return rc;
}

} // namespace

std::vector<Test> qc_tests() {
    return {
        {"qc_sine", test_qc_sine},
        {"qc_silence", test_qc_silence},
        {"qc_head_tail_silence", test_qc_head_tail_silence},
        {"qc_invalid", test_qc_invalid},
    };
}
