#!/usr/bin/env bash
# setup.sh — one-click install of Game Audio Forge (idempotent).
#
# usage:  bash scripts/setup.sh          (fresh or re-run — everything cached)
# no sudo needed. requires: git, curl, cmake, ninja/make, gcc, nvidia driver.
#
# steps (all idempotent):
#   1. micromamba (user-space, stabil in ~/.local/bin)
#   2. conda-env "gaf": gcc/g++ 13 + sentencepiece
#   3. CUDA Toolkit 12.8 (Runfile → user-space, via scripts/setup-cuda.sh)
#   4. clone acestep.cpp + sa3.cpp + audiogen.cpp + CUDA build
#   5. model weights (ACE-Step SFT set, SA3 small-sfx, Dasheng Q4)
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

# --- 0) prerequisites -----------------------------------------------------
for t in git curl cmake g++ make nvidia-smi jq; do
  command -v "$t" >/dev/null || die "missing tool: $t (install via dnf)"
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
  bash "$GAF_HOME/scripts/setup-cuda.sh" || die "CUDA setup failed"
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
  step "clone acestep.cpp"
  git clone -q --recurse-submodules https://github.com/ServeurpersoCom/acestep.cpp.git "$ACE"
fi
if [[ ! -x "$ACE/build/ace-synth" ]]; then
  step "build acestep.cpp (sm_120a)"
  ( cd "$ACE" && mkdir -p build && cd build \
    && cmake .. -DGGML_CUDA=ON -DCMAKE_CUDA_COMPILER="$CUDA_DIR/bin/nvcc" \
         -DCMAKE_BUILD_TYPE=Release -DCMAKE_CUDA_ARCHITECTURES="120a-real" >/dev/null \
    && cmake --build . -j "$(nproc)" >/dev/null ) || die "acestep.cpp Build"
fi
ok "acestep.cpp gebaut"

SA3="$TOOLS/sa3.cpp"
if [[ ! -d "$SA3/.git" ]]; then
  step "clone sa3.cpp"
  git clone -q --depth 1 --recurse-submodules --shallow-submodules \
    https://github.com/betweentwomidnights/sa3.cpp.git "$SA3"
fi
if [[ ! -x "$SA3/build-cuda/bin/sa3-generate" ]]; then
  step "build sa3.cpp (CUDA)"
  ( cd "$SA3" && mkdir -p build-cuda && cd build-cuda \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DSA3_CUDA=ON \
         -DCMAKE_CUDA_COMPILER="$CUDA_DIR/bin/nvcc" \
         -DCMAKE_CUDA_ARCHITECTURES="120a-real" >/dev/null \
    && cmake --build . -j "$(nproc)" >/dev/null \
    || cmake --build . -j "$(nproc)" >/dev/null ) || die "sa3.cpp Build"
fi
ok "sa3.cpp gebaut"

if [[ ! -d "$AUD/.git" ]]; then
  step "clone audiogen.cpp (legacy backend)"
  git clone -q --recurse-submodules https://github.com/audiohacking/audiogen.cpp.git "$AUD"
fi
if [[ ! -x "$AUD/build-cuda/dasheng-audiogen" ]]; then
  step "build audiogen.cpp (legacy backend)"
  ( cd "$AUD" && cmake -B build-cuda -S . -DCMAKE_BUILD_TYPE=Release \
      -DDASHENG_AUDIOGEN_METAL=OFF -DDASHENG_AUDIOGEN_CUDA=ON \
      -DCMAKE_CUDA_COMPILER="$CUDA_DIR/bin/nvcc" >/dev/null \
    && cmake --build build-cuda --parallel >/dev/null ) || die "audiogen.cpp Build"
fi
ok "audiogen.cpp gebaut (legacy)"

# --- 5) models ---------------------------------------------------------------
have() { [[ -s "$1" ]];
  sz=$(wc -c < "$1" 2>/dev/null || echo 0); (( sz > 1000000 )); }
M_ACE="$ACE/models"; M_AUD="$AUD/models"; M_SA3="$SA3/models"
mkdir -p "$M_ACE" "$M_AUD" "$M_SA3"
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

need_sa3=0
for f in t5gemma-b-b-ul2-encoder-0.3B-v1.0-F16.gguf t5gemma-b-b-ul2-v1.0-vocab.gguf \
         stable-audio-3-small-sfx-dit-0.5B-v1.0-Q8_0.gguf \
         stable-audio-3-small-sfx-same-s-v1.0-F32.gguf \
         stable-audio-3-small-sfx-conditioner-v1.0-F32.gguf; do
  have "$M_SA3/$f" || need_sa3=1
done
if (( need_sa3 )); then
  step "Stable Audio 3 small-sfx Q8 GGUF (~2.6 GB)"
  for f in t5gemma-b-b-ul2-encoder-0.3B-v1.0-F16.gguf t5gemma-b-b-ul2-v1.0-vocab.gguf \
           stable-audio-3-small-sfx-dit-0.5B-v1.0-Q8_0.gguf \
           stable-audio-3-small-sfx-same-s-v1.0-F32.gguf \
           stable-audio-3-small-sfx-conditioner-v1.0-F32.gguf; do
    hf download --quiet thepatch/stable-audio-3-small-sfx-GGUF "$f" --local-dir "$M_SA3" \
      || hf download --quiet thepatch/t5gemma-b-b-ul2-GGUF "$f" --local-dir "$M_SA3" \
      || die "Download $f"
  done
fi
ok "SA3 small-sfx models"

need_aud=0
for f in t5_encoder.gguf dit_Q4_0.gguf vocoder.gguf spiece.model; do
  have "$M_AUD/$f" || need_aud=1
done
if (( need_aud )); then
  step "Dasheng Q4 models, legacy backend (~2.8 GB)"
  for f in t5_encoder.gguf dit_Q4_0.gguf vocoder.gguf spiece.model; do
    hf download --quiet audiohacking/dasheng-audiogen-gguf "$f" --local-dir "$M_AUD" || die "Download $f"
  done
fi
[[ -f "$M_AUD/dit.gguf" || ! -f "$M_AUD/dit_Q4_0.gguf" ]] || mv "$M_AUD/dit_Q4_0.gguf" "$M_AUD/dit.gguf"
ok "Dasheng models (legacy)"

# --- 6) Smoke -----------------------------------------------------------------
step "smoke test"
[[ "$("$GAF_HOME/scripts/gaf" status | grep -c OK)" -ge 8 ]] || true
"$GAF_HOME/scripts/gaf" status

printf '\n\033[1;32m[setup] DONE.\033[0m  usage: %s/scripts/gaf music "…" | gaf sfx "…" | gaf ui | gaf status\n' "$GAF_HOME"
echo "service (UI, auto-start after reboot): bash $GAF_HOME/scripts/gaf-serve-install.sh"
