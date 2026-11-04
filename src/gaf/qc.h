#pragma once
#include <string>

namespace gaf {

// Results of a quality-control pass over a PCM WAV file.
struct QcResult {
    bool valid = false;        // file parsed as a PCM WAV
    std::string error;         // set when valid == false
    int sample_rate = 0;
    int channels = 0;
    int bit_depth = 0;         // 16 or 32 (float)
    double duration_s = 0.0;
    double peak = 0.0;         // max |sample|, 0..1
    double rms = 0.0;          // root-mean-square, 0..1
    double silence_head_s = 0.0;  // leading silence (below threshold)
    double silence_tail_s = 0.0;  // trailing silence
    // Approximate loudness in LUFS (integrated, ITU-R BS.1770-ish:
    // K-weighted gate approximated by RMS with a -70 dB floor). Good enough
    // for QC comparisons, not a certified loudness meter.
    double loudness_lufs = 0.0;
};

// Analyze a PCM WAV file (16-bit int or 32-bit float, any channel count).
// `silence_threshold` is linear amplitude (default 0.003 ≈ -50 dBFS).
QcResult analyze_wav(const std::string& path, double silence_threshold = 0.003);

} // namespace gaf
