#!/usr/bin/env python3
"""gaf-quality.py — objective audio analysis for generated files.

usage:
    gaf-quality.py FILE [FILE2]

output (JSON, one line):
    bpm               measured tempo (onset autocorrelation, 60–240)
    bpm_conf          peak/median autocorrelation (confidence, >1.5 = solid)
    duration_s        actual length
    rate              Abtastrate
    rms_db / peak_db  loudness (overall)
    stereo_corr       Median-Korrelation L/R (astats)
    hf_delta_db       RMS >4 kHz minus total RMS (negative = HF-poor)
    envelope_corr     Pearson of onset envelopes, 2 files only (0..1)

Pure Python-Stdlib + ffmpeg/ffprobe.
"""
import json
import math
import struct
import subprocess
import sys

RATE = 8000       # Analyse-Samplingrate
BLOCK = 0.02      # block length s (20 ms → resolves BPM up to 240)
MAXSEC = 180      # Analyse-Fenster


def run(cmd, binary=False):
    p = subprocess.run(cmd, capture_output=True, check=True)
    out = p.stderr  # ffmpeg-Statistiken gehen nach stderr
    return out if binary else out.decode("utf-8", "replace")


def run_out(cmd):
    p = subprocess.run(cmd, capture_output=True, check=True)
    return p.stdout


def meta(path):
    s = run_out(["ffprobe", "-v", "error", "-show_entries",
             "stream=sample_rate,channels", "-show_entries", "format=duration",
             "-of", "default=nw=1", path]).decode()
    d = {}
    for line in s.splitlines():
        k, _, v = line.partition("=")
        d[k] = v
    return d


def rms_db(path, chain=None):
    af = ["-af", f"{chain},astats=measure_overall=RMS_level" if chain
          else "astats=measure_overall=RMS_level"]
    out = run(["ffmpeg", "-hide_banner", "-i", path, *af, "-f", "null", "-"],
              binary=True).decode("utf-8", "replace")
    for line in out.splitlines():
        if "RMS level dB" in line:
            return float(line.split(":")[-1])
    return None


def peak_db(path):
    out = run(["ffmpeg", "-hide_banner", "-i", path,
               "-af", "astats=measure_overall=Peak_level",
               "-f", "null", "-"], binary=True).decode("utf-8", "replace")
    for line in out.splitlines():
        if "Peak level dB" in line:
            return float(line.split(":")[-1])
    return None


def stereo_corr(path):
    raw = run_out(["ffmpeg", "-v", "error", "-i", path, "-ac", "2",
               "-ar", str(RATE), "-f", "s16le", "-"])
    n = min(len(raw) // 4, MAXSEC * RATE)
    if n < 100:
        return None
    samples = struct.unpack(f"<{2 * n}h", raw[:n * 4])
    l = [samples[2 * i] for i in range(n)]
    r = [samples[2 * i + 1] for i in range(n)]
    return pearson(l, r)


def envelope(path):
    raw = run_out(["ffmpeg", "-v", "error", "-i", path, "-ac", "1",
               "-ar", str(RATE), "-f", "s16le", "-"])
    n = min(len(raw) // 2, MAXSEC * RATE)
    samples = struct.unpack(f"<{n}h", raw[:n * 2])
    bl = int(RATE * BLOCK)
    env = []
    for i in range(0, n - bl, bl):
        s = samples[i:i + bl]
        env.append(math.sqrt(sum(x * x for x in s) / bl))
    return env


def measure_bpm(env):
    if len(env) < 200:
        return None, 0.0
    flux = [max(0.0, env[i] - env[i - 1]) for i in range(1, len(env))]
    m = max(flux) or 1.0
    flux = [f / m for f in flux]
    best, best_score, scores = None, -1.0, []
    for bpm in range(60, 241):
        lag = round(60 / bpm / BLOCK)
        score = sum(sum(flux[i] * flux[i + lag * k] for i in range(len(flux) - lag * k))
                    for k in (1, 2, 3)) / 3
        scores.append(score)
        if score > best_score:
            best, best_score = bpm, score
    med = sorted(scores)[len(scores) // 2] or 1e-9
    return best, round(best_score / med, 2)


def pearson(a, b):
    n = min(len(a), len(b))
    a, b = a[:n], b[:n]
    ma, mb = sum(a) / n, sum(b) / n
    num = sum((x - ma) * (y - mb) for x, y in zip(a, b))
    da = math.sqrt(sum((x - ma) ** 2 for x in a))
    db = math.sqrt(sum((y - mb) ** 2 for y in b))
    return round(num / (da * db), 3) if da and db else None


def analyze(path):
    m = meta(path)
    env = envelope(path)
    rms = rms_db(path)
    pk = peak_db(path)
    hf = rms_db(path, "highpass=f=4000")
    bpm, conf = measure_bpm(env)
    return {
        "duration_s": round(float(m.get("duration", 0)), 2),
        "rate": int(m.get("sample_rate", 0)),
        "bpm": bpm,
        "bpm_conf": conf,
        "rms_db": round(rms, 1) if rms is not None else None,
        "peak_db": round(pk, 1) if pk is not None else None,
        "stereo_corr": stereo_corr(path),
        "hf_delta_db": round(hf - rms, 1) if hf is not None and rms is not None else None,
    }


def main():
    files = sys.argv[1:]
    if not files:
        print(__doc__, file=sys.stderr)
        sys.exit(1)
    out = analyze(files[0])
    if len(files) > 1:
        out["envelope_corr"] = pearson(envelope(files[0]), envelope(files[1]))
    print(json.dumps(out))


if __name__ == "__main__":
    main()
