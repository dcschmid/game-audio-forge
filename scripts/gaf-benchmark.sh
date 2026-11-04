#!/usr/bin/env bash
# gaf-benchmark.sh — measure RTF and peak VRAM for a generation.
#
# Usage:
#   scripts/gaf-benchmark.sh <model> <prompt> [duration_s] [seed]
#
# Samples nvidia-smi memory.used once per second while the generation runs,
# then reports peak VRAM and the engine-reported RTF.
set -euo pipefail

MODEL="${1:?model id, e.g. ace_step or midashenglm_gen}"
PROMPT="${2:?prompt text}"
DURATION="${3:-24}"
SEED="${4:-0}"

GAF="${GAF:-$(cd "$(dirname "$0")/.." && pwd)/build/gaf}"
if [[ ! -x "$GAF" ]]; then
  echo "gaf binary not found at $GAF — build first (cmake --build build)" >&2
  exit 1
fi

VRAM_LOG="$(mktemp)"
trap 'kill "$VRAM_PID" 2>/dev/null || true; rm -f "$VRAM_LOG"' EXIT

# Sample VRAM in the background.
( while :; do nvidia-smi --query-gpu=memory.used --format=csv,noheader,nounits 2>/dev/null || echo 0; sleep 1; done ) > "$VRAM_LOG" &
VRAM_PID=$!

echo "model=$MODEL duration=${DURATION}s seed=$SEED"
echo "prompt: $PROMPT"
echo "---"

# Time the generation; capture stderr (timing) separately.
START=$(date +%s.%N)
OUT="$("$GAF" generate "$MODEL" --text "$PROMPT" --duration "$DURATION" --seed "$SEED" \
        --profile benchmark 2>/tmp/gaf_bench_err.txt)"
RC=$?
END=$(date +%s.%N)
WALL=$(awk "BEGIN{printf \"%.2f\", $END-$START}")

PEAK=$(sort -n "$VRAM_LOG" | tail -1)
echo "wall_time=${WALL}s"
echo "peak_vram_mib=${PEAK}"
echo "output=${OUT}"
echo "--- engine timing ---"
cat /tmp/gaf_bench_err.txt
exit $RC
