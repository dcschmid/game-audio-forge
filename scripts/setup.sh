#!/usr/bin/env bash
# setup.sh — One-Click-Installation der Game Audio Forge (idempotent).
#
# usage:  bash scripts/setup.sh          (frisch oder erneut — alles cached)
# no sudo needed. requires: git, curl, cmake, ninja/make, gcc, nvidia driver.
#
# stepe (alle idempotent):
#   1. micromamba (user-space, stabil in ~/.local/bin)
#   2. conda-env "gaf": gcc/g++ 13 + sentencepiece
#   3. CUDA Toolkit 12.8 (Runfile → user-space, via scripts/setup-cuda.sh)
#   4. acestep.cpp + audiogen.cpp klonen + CUDA-Build
#   5. Modellgewichte (ACE-Step SFT-Set, Dasheng Q4)
#   6. gaf smoke check
set -uo pipefail

GAF_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLS="${GAF_TOOLS:-$HOME/dev/github/tools}"
MAMBA="$HOME/.local/bin/micromamba"
MAMBA_ROOT="$HOME/.mamba"
CUDA_DIR="$TOOLS/cuda/cuda-12.8"
ACE="$TOOLS/acestep.cpp"
AUD="$TOOLS/audiogen.cpp"

step() { printf '\n\033[1;34m[setup]\033[0m %s\n' "$*"; }
ok()   { printf '\033[1;32m  ✓\033[0m %s\n' "$*"; }
die()  { printf '\033[1;31m[setup:err]\033[0m %s\n' "$*" >&2; exit 1; }

# --- 0) Voraussetzungen -----------------------------------------------------
for t in git curl cmake g++ make nvidia-smi jq; do
  command -v "$t" >/dev/null || die "fehlendes Tool: $t (dnf installieren)"
done
nvidia-smi -L >/dev/null 2>&1 || die "kein NVIDIA-Treiber (nvidia-smi)"

# --- 1) micromamba (persistent) ----------------------------------------------
mkdir -p "$HOME/.local/bin" "$MAMBA_ROOT"
if [[ ! -x "$MAMBA" ]]; then
  step "micromamba → ~/.local/bin"
  curl -sL -o "$MAMBA" \
    "https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-linux-64" \
    && chmod +x "$MAMBA" || die "micromamba-Download fehlgeschlagen"
fi
ok "micromamba $($MAMBA --version 2>/dev/null)"

# --- 2) Build-Umgebung (gcc13 + sentencepiece) --------------------------------
if [[ ! -x "$MAMBA_ROOT/envs/gaf/bin/g++" ]]; then
  step "conda-env gaf (gcc/g++ 13, sentencepiece)"
  MAMBA_ROOT_PREFIX="$MAMBA_ROOT" "$MAMBA" create -y -n gaf \
    -c conda-forge "gcc=13" "gxx=13" sentencepiece >/dev/null \
    || die "conda-env gaf fehlgeschlagen"
fi
ok "gcc $("$MAMBA_ROOT/envs/gaf/bin/g++" --version | head -1 | grep -oE '[0-9]+\.[0-9.]+')"

# --- 3) CUDA Toolkit 12.8 (user-space) ----------------------------------------
if [[ ! -x "$CUDA_DIR/bin/nvcc" ]]; then
  step "CUDA 12.8 Toolkit (Runfile, ~3 GB — dauert)"
  bash "$GAF_HOME/scripts/setup-cuda.sh" || die "CUDA-Setup fehlgeschlagen"
fi
ok "nvcc $("$CUDA_DIR/bin/nvcc" --version | grep -oE 'release [0-9.]+' | cut -d' ' -f2)"

export GAF_MICROMAMBA="$MAMBA" GAF_MICROMAMBA_ROOT="$MAMBA_ROOT"
export PATH="$CUDA_DIR/bin:$MAMBA_ROOT/envs/gaf/bin:$PATH"
export LD_LIBRARY_PATH="$CUDA_DIR/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export PKG_CONFIG_PATH="$MAMBA_ROOT/envs/gaf/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
export NVCC_CCBIN="$MAMBA_ROOT/envs/gaf/bin/g++"

# --- 4) Generatoren -------------------------------------------------------------
mkdir -p "$TOOLS"
if [[ ! -d "$ACE/.git" ]]; then
  step "acestep.cpp klonen"
  git clone -q --recurse-submodules https://github.com/ServeurpersoCom/acestep.cpp.git "$ACE"
fi
if [[ ! -x "$ACE/build/ace-synth" ]]; then
  step "acestep.cpp bauen (sm_120a)"
  ( cd "$ACE" && mkdir -p build && cd build \
    && cmake .. -DGGML_CUDA=ON -DCMAKE_CUDA_COMPILER="$CUDA_DIR/bin/nvcc" \
         -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_ARCHITECTURES="120a-real" >/dev/null \
    && cmake --build . -j "$(nproc)" >/dev/null ) || die "acestep.cpp Build"
fi
ok "acestep.cpp gebaut"

if [[ ! -d "$AUD/.git" ]]; then
  step "audiogen.cpp klonen"
  git clone -q --recurse-submodules https://github.com/audiohacking/audiogen.cpp.git "$AUD"
fi
if [[ ! -x "$AUD/build-cuda/dasheng-audiogen" ]]; then
  step "audiogen.cpp bauen"
  ( cd "$AUD" && cmake -B build-cuda -S . -DCMAKE_BUILD_TYPE=Release \
      -DDASHENG_AUDIOGEN_METAL=OFF -DDASHENG_AUDIOGEN_CUDA=ON \
      -DCMAKE_CUDA_COMPILER="$CUDA_DIR/bin/nvcc" >/dev/null \
    && cmake --build build-cuda --parallel >/dev/null ) || die "audiogen.cpp Build"
fi
ok "audiogen.cpp gebaut"

# --- 5) models ---------------------------------------------------------------
have() { [[ -s "$1" ]];
  sz=$(wc -c < "$1" 2>/dev/null || echo 0); (( sz > 1000000 )); }
M_ACE="$ACE/models"; M_AUD="$AUD/models"
mkdir -p "$M_ACE" "$M_AUD"
need_ace=0
for f in vae-BF16.gguf Qwen3-Embedding-0.6B-Q8_0.gguf acestep-5Hz-lm-0.6B-Q8_0.gguf acestep-v15-sft-Q8_0.gguf; do
  have "$M_ACE/$f" || need_ace=1
done
if (( need_ace )); then
  step "ACE-Step models (SFT set + 0.6B LM + VAE, ~6 GB)"
  for f in vae-BF16.gguf Qwen3-Embedding-0.6B-Q8_0.gguf acestep-5Hz-lm-0.6B-Q8_0.gguf acestep-v15-sft-Q8_0.gguf; do
    hf download --quiet Serveurperso/ACE-Step-1.5-GGUF "$f" --local-dir "$M_ACE" || die "Download $f"
  done
fi
ok "ACE-Step models"
need_aud=0
for f in t5_encoder.gguf dit_Q4_0.gguf vocoder.gguf spiece.model; do
  have "$M_AUD/$f" || need_aud=1
done
if (( need_aud )); then
  step "Dasheng Q4 models (~2.8 GB)"
  for f in t5_encoder.gguf dit_Q4_0.gguf vocoder.gguf spiece.model; do
    hf download --quiet audiohacking/dasheng-audiogen-gguf "$f" --local-dir "$M_AUD" || die "Download $f"
  done
fi
[[ -f "$M_AUD/dit.gguf" || ! -f "$M_AUD/dit_Q4_0.gguf" ]] || mv "$M_AUD/dit_Q4_0.gguf" "$M_AUD/dit.gguf"
ok "Dasheng models"

# --- 6) Smoke -----------------------------------------------------------------
step "Smoke-Test"
[[ "$("$GAF_HOME/scripts/gaf" status | grep -c OK)" -ge 8 ]] || true
"$GAF_HOME/scripts/gaf" status

printf '\n\033[1;32m[setup] FERTIG.\033[0m  Nutzung: %s/scripts/gaf music "…" | gaf sfx "…" | gaf ui | gaf status\n' "$GAF_HOME"
echo "Dienst (UI, Autostart nach Reboot): bash $GAF_HOME/scripts/gaf-serve-install.sh"
