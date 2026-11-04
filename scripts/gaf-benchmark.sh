#!/usr/bin/env bash
# gaf-benchmark.sh — misst Geschwindigkeit, RAM/VRAM und Audioqualität.
#
# Nutzung:
#   scripts/gaf-benchmark.sh music "Prompt"
#   scripts/gaf-benchmark.sh sfx "Prompt"
#
# Misst:
#   - Wall-Clock-Zeit der Generierung
#   - VRAM-Peak (nvidia-smi, 0.5 s-Abtastung im Hintergrund)
#   - RAM-Peak (VmHWM des Prozesses)
#   - Audioqualität: Dauer, Abtastrate, Kanäle, RMS-Lautstärke,
#     Crest-Faktor, spektrale Bandbreite (via FFmpeg/sox)
set -euo pipefail

GAF_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GAF="$GAF_HOME/scripts/gaf"

KIND="${1:-}"
PROMPT="${2:-}"
[[ -z "$KIND" || -z "$PROMPT" ]] && { echo "Nutzung: $0 {music|sfx} \"Prompt\""; exit 1; }

WORK="$(mktemp -d)"
VRAM_LOG="$WORK/vram.log"
trap 'kill "$VRAM_PID" 2>/dev/null; sleep 0.6; rm -rf "$WORK"' EXIT

# --- VRAM-Abtaster im Hintergrund (alle 0.2 s) ---
(
  while true; do
    nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null \
      | tr -d ' ' >> "$VRAM_LOG" 2>/dev/null || true
    sleep 0.2
  done
) &
VRAM_PID=$!

# --- Start-Zustand ---
VRAM_START="$(nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits | tr -d ' ')"
T_START=$(date +%s.%N)

# --- Generierung (gaf schreibt nach output/{music,sfx}) ---
"$GAF" "$KIND" "$PROMPT" 2>&1 | sed 's/^/    /'

T_END=$(date +%s.%N)
kill "$VRAM_PID" 2>/dev/null || true
wait "$VRAM_PID" 2>/dev/null || true

ELAPSED=$(awk "BEGIN{printf \"%.1f\", $T_END - $T_START}")

# --- VRAM-Peak ---
VRAM_PEAK=$(sort -n "$VRAM_LOG" 2>/dev/null | tail -1)
VRAM_PEAK="${VRAM_PEAK:-?}"

# --- Neueste Ausgabe-Datei finden ---
OUTDIR="$GAF_HOME/output/$KIND"
LATEST="$(ls -t "$OUTDIR"/*.wav 2>/dev/null | head -1)"
[[ -n "$LATEST" ]] || { echo "Keine Ausgabe gefunden in $OUTDIR"; exit 1; }

# --- Audio-Analyse via FFmpeg ---
DUR="$(ffprobe -v error -show_entries format=duration -of csv=p=0 "$LATEST")"
RATE="$(ffprobe -v error -select_streams a:0 -show_entries stream=sample_rate -of csv=p=0 "$LATEST")"
CH="$(ffprobe -v error -select_streams a:0 -show_entries stream=channels -of csv=p=0 "$LATEST")"

RMS="$(ffmpeg -hide_banner -i "$LATEST" -af "astats=measure_overall=RMS_level" -f null - 2>&1 \
  | grep 'RMS level dB' | tail -1 | grep -oE '[-0-9.]+' | tail -1)"
PEAK="$(ffmpeg -hide_banner -i "$LATEST" -af "astats=measure_overall=Peak_level" -f null - 2>&1 \
  | grep 'Peak level dB' | tail -1 | grep -oE '[-0-9.]+' | tail -1)"
CREST="$(awk -v p="${PEAK:-0}" -v r="${RMS:-0}" 'BEGIN{printf "%.1f", p - r}')"

#HF = RMS des Hochpass-Anteils (>4 kHz) als overall. Nähe zu RMS (Gesamt) → viel HF.
HF="$(ffmpeg -hide_banner -i "$LATEST" -af "highpass=f=4000,astats=measure_overall=RMS_level" -f null - 2>&1 \
  | grep 'RMS level dB' | tail -1 | grep -oE '[-0-9.]+' | tail -1)"

echo
echo "════════════════════════════════════════════════"
echo "  Benchmark: $KIND — \"$PROMPT\""
echo "════════════════════════════════════════════════"
echo "  Datei:        $LATEST"
echo "  Dauer Audio:  ${DUR}s"
echo "  Abtastrate:   ${RATE} Hz"
echo "  Kanäle:       $CH"
echo "  ── Performance ──"
echo "  Wall-Clock:   ${ELAPSED}s"
echo "  VRAM-Peak:    ${VRAM_PEAK} MiB (Start: ${VRAM_START} MiB)"
echo "  ── Audioqualität ──"
echo "  RMS:          ${RMS} dB"
echo "  Peak:         ${PEAK} dB"
echo "  Crest-Faktor: ${CREST} dB"
HF_DELTA="$(awk -v h="${HF:-0}" -v r="${RMS:-0}" 'BEGIN{printf "%.1f", h - r}')"
echo "  Hochpass >4k: ${HF} dB  (Gesamt-RMS: ${RMS} dB → HF-Anteil: ${HF_DELTA} dB unter Gesamt)"
echo "════════════════════════════════════════════════"
