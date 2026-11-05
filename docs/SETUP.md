# Setup

Target: Fedora 44, Framework Laptop 16, NVIDIA RTX 5070 Laptop (8 GB VRAM,
sm_120 / Blackwell).

**Short answer:** `bash scripts/setup.sh`. It is idempotent, needs no sudo,
and finishes a cold install in minutes or a warm re-run in under a minute.
This page explains what it does and why.

## Prerequisites

- NVIDIA driver (here: 615.78, UMD 13.4) — installed
- CMake ≥ 3.18, GCC/G++, git, make/ninja, pkg-config, jq — installed
- **CUDA Toolkit 12.8** into user space — because the system package manager
  path needs sudo and this machine's sudo wants a fingerprint prompt
- **GCC 13** (conda-forge) — Fedora 44 ships GCC 16, and nvcc 12.8 cannot
  compile with it (`char8_t` errors; see
  [acestep.cpp #72](https://github.com/ServeurpersoCom/acestep.cpp/issues/72))
- **sentencepiece** (conda-forge) — audiogen.cpp dependency
- `hf` CLI (HuggingFace) — model downloads
- FFmpeg (optional) — resampling/transcoding

## Why not just install system-wide?

Two reasons, both boring and both real:

1. sudo on this machine resolves via fingerprint, which times out in headless
   sessions.
2. Everything below lives happily in `~` — nothing to break, nothing to
   uninstall, easy to wipe and redo.

## 1. CUDA Toolkit (user space, no sudo)

The NVIDIA runfile installer insists on a GUI terminal (konsole) for its EULA.
Workaround: extract the payload and merge the components by hand
(`scripts/setup-cuda.sh`):

```bash
cd ~/dev/github/tools/cuda
curl -sL -o cuda_12.8.1_linux.run \
  https://developer.download.nvidia.com/compute/cuda/12.8.1/local_installers/cuda_12.8.1_570.124.06_linux.run
env -u DISPLAY -u WAYLAND_DISPLAY TERM=dumb \
  sh cuda_12.8.1_linux.run --extract=$PWD/extracted < /dev/null
# merge components into cuda-12.8/{bin,lib64,include,nvvm} → setup-cuda.sh
```

Result: `~/dev/github/tools/cuda/cuda-12.8/{bin,lib64,include,nvvm}`.

> **Why 12.8 and not 13.x?** CUDA 13.x supports host compilers only up to
> GCC 15. And 12.8 knows `sm_120a` (RTX 5070) but not `sm_121a` (added in
> 12.9). So: build with an explicit `-DCMAKE_CUDA_ARCHITECTURES=120a-real`.

## 2. GCC 13 + sentencepiece (conda-forge, no sudo)

```bash
curl -sL -o ~/.local/bin/micromamba \
  https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-linux-64
chmod +x ~/.local/bin/micromamba
MAMBA_ROOT_PREFIX=$HOME/.mamba ~/.local/bin/micromamba create -y -n gaf \
  -c conda-forge "gcc=13" "gxx=13" sentencepiece
```

## 3. Load the environment

```bash
source ~/dev/github/game-audio-forge/scripts/gaf-env
```

Sets `PATH` (nvcc + conda-gaf), `LD_LIBRARY_PATH`, `PKG_CONFIG_PATH` and
`NVCC_CCBIN` (→ GCC 13). Run it in every shell before using `gaf` or builds.

## 4. Build acestep.cpp (music)

```bash
cd ~/dev/github/tools/acestep.cpp
mkdir -p build && cd build
cmake .. -DGGML_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$GAF_CUDA/bin/nvcc" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CUDA_ARCHITECTURES="120a-real"
cmake --build . -j "$(nproc)"
```

Binaries: `build/{ace-lm,ace-synth,ace-server,ace-understand,neural-codec,mp3-codec,quantize}`.

## 5. Build sa3.cpp (SFX, default backend)

```bash
cd ~/dev/github/tools/sa3.cpp
cmake -B build-cuda -S . -DCMAKE_BUILD_TYPE=Release -DSA3_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$GAF_CUDA/bin/nvcc" \
  -DCMAKE_CUDA_ARCHITECTURES="120a-real"
cmake --build build-cuda --parallel
```

Binary: `build-cuda/bin/sa3-generate` (shared `libsa3.so` → run with
`build-cuda` on `LD_LIBRARY_PATH`; `gaf` does this itself). sa3.cpp's CMake
adds `120a-real` automatically on CUDA ≥ 12.8. One test target may fail to
link while every CLI binary builds — `gaf` only needs the CLI.

Legacy backend (optional, `--backend dasheng`):

```bash
cd ~/dev/github/tools/audiogen.cpp
cmake -B build-cuda -S . -DCMAKE_BUILD_TYPE=Release \
  -DDASHENG_AUDIOGEN_METAL=OFF -DDASHENG_AUDIOGEN_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$GAF_CUDA/bin/nvcc"
cmake --build build-cuda --parallel
```

Binary: `build-cuda/dasheng-audiogen`. audiogen's ggml 0.15.2 gates `121a`
behind CUDA ≥ 12.9, so with 12.8 it builds `120a-real` on its own — no
override needed.

## 6. Models

```bash
# music: ACE-Step 1.5 SFT DiT + 0.6B LM + text-encoder + VAE (Q8_0, ~6.5 GB)
cd ~/dev/github/tools/acestep.cpp
for f in vae-BF16.gguf Qwen3-Embedding-0.6B-Q8_0.gguf \
         acestep-5Hz-lm-0.6B-Q8_0.gguf acestep-v15-sft-Q8_0.gguf; do
  hf download --quiet Serveurperso/ACE-Step-1.5-GGUF "$f" --local-dir models
done

# SFX default: Stable Audio 3 small-sfx Q8 GGUF (~2.6 GB)
cd ~/dev/github/tools/sa3.cpp
./models.sh --variant small-sfx --encoding q8_0

# SFX legacy: Dasheng AudioGen Q4 (~2.8 GB)
cd ~/dev/github/tools/audiogen.cpp
make download-models-q4
```

## 7. Verify

```bash
gaf status        # everything OK?
scripts/gaf-test.sh   # 7 deterministic generations, ~70 s, 7/7
```

## VRAM budget (8 GB)

| Generator | Weights on disk | Resident VRAM |
|---|---|---|
| acestep.cpp | SFT DiT Q8_0 2.4 GB + 0.6B LM Q8_0 0.7 GB + text-enc 0.7 GB + VAE 0.3 GB ≈ 4.1 GB | ~4.5 GB peak (module-by-module eviction) |
| sa3.cpp (small-sfx Q8) | DiT 0.5B Q8_0 1.7 GB + SAME-S F32 0.4 GB + T5Gemma F16 0.5 GB ≈ 2.6 GB | ~2.5 GB peak |
| audiogen.cpp (legacy) | T5 F32 1.4 GB + DiT Q4 1.2 GB + vocoder 0.3 GB ≈ 2.9 GB | ~1.8 GB peak |

Each fits comfortably on its own. **Never run them in parallel** — `gaf`
queues generator processes for you. Measured: a 30 s track takes ~9 s wall
and ~4.5 GB peak, regardless of length, because the VAE decodes in tiles.
