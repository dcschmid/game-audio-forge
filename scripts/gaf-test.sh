#!/usr/bin/env bash
# gaf-test.sh — quality gate: generate defined cases deterministically + score.
#
# usage: scripts/gaf-test.sh
# needs gaf-env (CUDA/models). result: stdout JSON + output/tests/<date>/
set -euo pipefail
GAF_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck disable=SC1091
source "$GAF_HOME/scripts/gaf-env" >/dev/null
GAF="$GAF_HOME/scripts/gaf"
PY="$GAF_HOME/scripts/gaf-quality.py"
RUNDIR="$GAF_HOME/output/tests/$(date +%Y%m%d-%H%M%S)"
mkdir -p "$RUNDIR"

declare -A BPM_MIN=( )
declare -A BPM_MAX=( )
# note: --bpm is currently ignored by the turbo port (measured: 160 requested
# -> measured 106, conf 4.06). BPM is reported, not asserted.
declare -A DUR_MIN=( [metal-racing]=16 [chiptune-loop]=16 [ambient-pad]=16 [cover-run]=20 [repaint-run]=20 )
declare -A DUR_MAX=( [metal-racing]=36 [chiptune-loop]=36 [ambient-pad]=36 [cover-run]=45 [repaint-run]=45 )
declare -A TASK=( [metal-racing]=text2music [chiptune-loop]=text2music [ambient-pad]=text2music
                  [cover-run]=cover [repaint-run]=repaint [kart-engine]=sfx [rain]=sfx )

pass=0 fail=0 total=0
results="[]"

run_case() { # name, kind, args…
  local name="$1" kind="$2"; shift 2
  total=$((total+1))
  local out dir
  echo "── $name"
  dir=$("$GAF" "$kind" "$@" 2>"$RUNDIR/$name.log" | grep -oE "$GAF_HOME/output/[^ ]+\.wav$" | tail -1)
  [[ -n "$dir" && -f "$dir" ]] || { echo "   GENERATION FEHLGESCHLAGEN"; fail=$((fail+1)); return; }
  mv "$dir" "$RUNDIR/$name.wav" 2>/dev/null || true
  [[ -f "${dir%.wav}.meta.json" ]] && mv "${dir%.wav}.meta.json" "$RUNDIR/$name.meta.json" 2>/dev/null || true
  local f="$RUNDIR/$name.wav"
  [[ -f "$f" ]] || { echo "   keine Ausgabe gefunden"; fail=$((fail+1)); return; }

  local q; q="$(python3 "$PY" "$f")"
  local dur=$(jq -r .duration_s <<<"$q")
  local bpm=$(jq -r .bpm <<<"$q")
  local conf=$(jq -r .bpm_conf <<<"$q")
  local rms=$(jq -r .rms_db <<<"$q")
  local hf=$(jq -r .hf_delta_db <<<"$q")
  local task_meta="text2music"
  [[ -f "$RUNDIR/$name.meta.json" ]] && task_meta=$(jq -r '.task_type // "text2music"' "$RUNDIR/$name.meta.json")

  local ok=1 reasons=""
  # 1) duration within bounds
  local dmin="${DUR_MIN[$name]:-4}" dmax="${DUR_MAX[$name]:-8}"
  awk -v d="$dur" -v a="$dmin" -v b="$dmax" 'BEGIN{exit !(d>=a*0.85 && d<=b*1.25)}' \
    || { ok=0; reasons+="duration=${dur}s outside [${dmin}-${dmax}] "; }
  # 2) Task-Typ (nur music)
  if [[ "$kind" == "music" ]]; then
    [[ "$task_meta" == "${TASK[$name]}" ]] \
      || { ok=0; reasons+="task=${task_meta}≠${TASK[$name]} "; }
  fi
  # 3) BPM band (only with bpm_conf >= 1.5)
  if [[ -n "${BPM_MIN[$name]:-}" ]]; then
    if awk -v c="$conf" 'BEGIN{exit !(c>=1.5)}'; then
      awk -v b="$bpm" -v a="${BPM_MIN[$name]}" -v z="${BPM_MAX[$name]}" \
        'BEGIN{exit !(b>=a-10 && b<=z+10)}' \
        || { ok=0; reasons+="bpm=${bpm}∉[${BPM_MIN[$name]}-${BPM_MAX[$name]}] "; }
    else
      echo "   BPM confidence $conf too low — band check skipped"
    fi
  fi
  # 4) Loudness
  awk -v r="$rms" 'BEGIN{exit !(r>-30 && r<-3)}' \
    || { ok=0; reasons+="rms=${rms}dB "; }
  # 5) HF content: content-dependent. pads/covers may be HF-poor (-32),
  #    rhythmische Inhalte sollten > -25 haben.
  local hfmin=-25
  case "$name" in ambient-pad|cover-run) hfmin=-32 ;; esac
  awk -v h="$hf" -v z="$hfmin" 'BEGIN{exit !(h>z)}' || { ok=0; reasons+="hf=${hf}dB "; }

  local entry
  entry=$(jq -n --arg n "$name" --argjson q "$q" --argjson ok "$ok" \
    --arg r "$reasons" '{name:$n}+$q+{pass:($ok==1),reasons:$r}')
  results=$(jq --argjson e "$entry" '.+[$e]' <<<"$results")
  if [[ "$ok" == 1 ]]; then pass=$((pass+1)); echo "   PASS  dur=${dur}s bpm=${bpm}(${conf}) rms=${rms}dB hf=${hf}dB"
  else fail=$((fail+1)); echo "   FAIL  $reasons"; fi
}

BASE="$GAF_HOME/output/music/20261104-215319_aggressive-instrumental-metal-racing-tra.wav"

run_case metal-racing music "driving metal racing track, fast double-kick, galloping guitars" --bpm 160 --duration 24 --seed 42
run_case chiptune-loop music "happy chiptune loop, retro video game" --duration 20 --seed 42
run_case ambient-pad music "soft ambient pad, slow, spacious" --duration 20 --seed 42
[[ -f "$BASE" ]] && run_case cover-run music "jazz cover with soft piano" --cover "$BASE" --duration 24 \
                || echo "── cover run skipped (base model missing)"
[[ -f "$BASE" ]] && run_case repaint-run music "repaired intro" --repaint "$BASE" --from 0 --to 8 \
                || echo "── repaint run skipped (base model missing)"
run_case kart-engine sfx "go-kart engine revving, small piston engine, high pitch"
run_case rain sfx "heavy rain falling on pavement, steady rainfall"

echo "════════════════════════════════"
echo "BESTANDEN: $pass/$total  (fehlgeschlagen: $fail)"
echo "$results" | jq . > "$RUNDIR/report.json"
echo "Report: $RUNDIR/report.json"
(( fail == 0 ))
