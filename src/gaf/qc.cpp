#include "gaf/qc.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

namespace gaf {

namespace {

uint32_t rd_u32le(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
           (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}
uint16_t rd_u16le(const uint8_t* p) {
    return static_cast<uint16_t>(p[0] | (p[1] << 8));
}

// Convert one interleaved frame of `channels` samples to float in [-1, 1].
// Returns false on an unsupported format.
bool frame_to_float(const uint8_t* p, int channels, int bit_depth,
                    std::vector<float>& out) {
    out.resize(static_cast<size_t>(channels));
    if (bit_depth == 16) {
        for (int c = 0; c < channels; ++c) {
            int16_t v = static_cast<int16_t>(
                static_cast<uint16_t>(p[c * 2]) | (static_cast<uint16_t>(p[c * 2 + 1]) << 8));
            out[c] = static_cast<float>(v) / 32768.0f;
        }
    } else if (bit_depth == 32) {
        // Assume IEEE-754 float (the format our engine emits for 32-bit).
        for (int c = 0; c < channels; ++c) {
            uint32_t bits = rd_u32le(p + c * 4);
            float f;
            std::memcpy(&f, &bits, 4);
            out[c] = f;
        }
    } else {
        return false;
    }
    return true;
}

} // namespace

QcResult analyze_wav(const std::string& path, double silence_threshold) {
    QcResult r;
    std::ifstream f(path, std::ios::binary);
    if (!f) { r.error = "cannot open: " + path; return r; }

    std::vector<uint8_t> buf((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());
    if (buf.size() < 44) { r.error = "file too small for a WAV header"; return r; }

    // RIFF header.
    if (std::memcmp(buf.data(), "RIFF", 4) != 0 ||
        std::memcmp(buf.data() + 8, "WAVE", 4) != 0) {
        r.error = "not a RIFF/WAVE file";
        return r;
    }

    // Walk chunks to find fmt + data.
    size_t off = 12;
    uint16_t audio_format = 0, channels = 0, bit_depth = 0;
    uint32_t sample_rate = 0;
    const uint8_t* data = nullptr;
    uint32_t data_bytes = 0;
    while (off + 8 <= buf.size()) {
        uint32_t chunk_size = rd_u32le(buf.data() + off + 4);
        const uint8_t* body = buf.data() + off + 8;
        if (off + 8 + chunk_size > buf.size()) chunk_size = static_cast<uint32_t>(buf.size() - off - 8);
        if (std::memcmp(buf.data() + off, "fmt ", 4) == 0 && chunk_size >= 16) {
            audio_format = rd_u16le(body);
            channels = rd_u16le(body + 2);
            sample_rate = rd_u32le(body + 4);
            bit_depth = rd_u16le(body + 14);
        } else if (std::memcmp(buf.data() + off, "data", 4) == 0) {
            data = body;
            data_bytes = chunk_size;
        }
        off += 8 + chunk_size + (chunk_size & 1); // chunks are word-aligned
    }
    if (!data || channels == 0 || sample_rate == 0) {
        r.error = "missing fmt/data chunk";
        return r;
    }
    if (audio_format != 1 && audio_format != 3) {
        r.error = "unsupported WAV format (need PCM or IEEE float)";
        return r;
    }
    if (bit_depth != 16 && bit_depth != 32) {
        r.error = "unsupported bit depth (need 16 or 32)";
        return r;
    }

    r.valid = true;
    r.sample_rate = static_cast<int>(sample_rate);
    r.channels = channels;
    r.bit_depth = bit_depth;

    size_t bytes_per_frame = static_cast<size_t>(channels) * (bit_depth / 8);
    size_t frames = data_bytes / bytes_per_frame;
    r.duration_s = static_cast<double>(frames) / static_cast<double>(sample_rate);

    // Single pass: peak, RMS, silence.
    double peak = 0.0, sum_sq = 0.0;
    size_t first_active = frames, last_active = 0;
    bool any_active = false;
    std::vector<float> frame;
    for (size_t i = 0; i < frames; ++i) {
        const uint8_t* p = data + i * bytes_per_frame;
        if (!frame_to_float(p, channels, bit_depth, frame)) {
            r.valid = false;
            r.error = "bad sample data";
            return r;
        }
        double max_abs = 0.0, sq = 0.0;
        for (float v : frame) {
            double a = std::fabs(static_cast<double>(v));
            if (a > max_abs) max_abs = a;
            sq += a * a;
        }
        if (max_abs > peak) peak = max_abs;
        sum_sq += sq;
        if (max_abs >= silence_threshold) {
            any_active = true;
            if (i < first_active) first_active = i;
            last_active = i;
        }
    }

    double n = static_cast<double>(frames * channels);
    r.peak = peak;
    r.rms = n > 0 ? std::sqrt(sum_sq / n) : 0.0;
    if (any_active) {
        r.silence_head_s = static_cast<double>(first_active) / sample_rate;
        r.silence_tail_s = static_cast<double>(frames - 1 - last_active) / sample_rate;
    } else {
        r.silence_head_s = r.duration_s;
        r.silence_tail_s = 0.0;
    }
    // Approximate integrated loudness: 10*log10(mean energy) with a -70 dBFS
    // floor, offset so full-scale sine ≈ -3 LUFS (close enough for QC).
    double energy = n > 0 ? sum_sq / n : 0.0;
    double dbfs = 10.0 * std::log10(energy > 1e-14 ? energy : 1e-14);
    r.loudness_lufs = dbfs + 3.0; // full-scale sine has -3 dBFS RMS -> ~0 LUFS
    return r;
}

} // namespace gaf
