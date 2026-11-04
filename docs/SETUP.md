# Setup

Lokale Audio-Werkstatt auf Fedora 44, Framework Laptop 16, RTX 5070 Laptop
(8 GB VRAM, sm_120 / Blackwell).

## Voraussetzungen

- NVIDIA-Treiber (hier: 615.78, UMD 13.4) — installiert.
- CMake ≥ 3.18, GCC/G++, git, ninja, pkg-config — installiert.
- **CUDA Toolkit 12.8** (user-space, sudo-frei) — siehe unten.
- **GCC 13** (conda-forge via micromamba) — Fedora 44 liefert GCC 16, mit dem
  nvcc (CUDA 12.8) nicht kompiliert (`char8_t`-Fehler, vgl.
  [acestep.cpp #72](https://github.com/ServeurpersoCom/acestep.cpp/issues/72)).
- **sentencepiece** (conda-forge) — nur für audiogen.cpp.
- `hf` CLI (HuggingFace) — für Modell-Downloads.
- FFmpeg (optional) — Resample/Transcode.

## 1. CUDA Toolkit (user-space, ohne sudo)

Der NVIDIA-Runfile-Installer will ein GUI-Terminal (konsole) für die EULA.
Workaround: Payload extrahieren und manuell mergen.

```bash
cd ~/dev/github/tools/cuda
# Runfile laden (12.8.1)
curl -sL -o cuda_12.8.1_linux.run \
  https://developer.download.nvidia.com/compute/cuda/12.8.1/local_installers/cuda_12.8.1_570.124.06_linux.run
# Payload extrahieren (ohne Installer)
env -u DISPLAY -u WAYLAND_DISPLAY TERM=dumb \
  sh cuda_12.8.1_linux.run --extract=$PWD/extracted < /dev/null
# Komponenten zu einem Toolkit mergen (siehe scripts/setup-cuda.sh)
```

Ergebnis: `~/dev/github/tools/cuda/cuda-12.8/{bin,lib64,include,nvvm}`.

> **Warum 12.8 und nicht 13.x?** CUDA 13.x unterstützt Host-Compiler nur bis
> GCC 15. Und CUDA 12.8 kennt `sm_120a` (RTX 5070), aber noch nicht `sm_121a`
> (das kam in 12.9). Deshalb beim Build explizit
> `-DCMAKE_CUDA_ARCHITECTURES=120a-real` setzen.

## 2. GCC 13 + sentencepiece (conda-forge, ohne sudo)

```bash
# micromamba (Standalone-Binary)
curl -sL -o /tmp/micromamba \
  https://github.com/mamba-org/micromamba-releases/releases/latest/download/micromamba-linux-64
chmod +x /tmp/micromamba
export MAMBA_ROOT_PREFIX=$HOME/.mamba
/tmp/micromamba create -y -n gaf -c conda-forge "gcc=13" "gxx=13" sentencepiece
```

## 3. Build-Umgebung laden

```bash
source ~/dev/github/game-audio-forge/scripts/gaf-env
```

Setzt `PATH` (nvcc + conda-gaf), `LD_LIBRARY_PATH`, `PKG_CONFIG_PATH` und
`NVCC_CCBIN` (→ GCC 13).

## 4. acestep.cpp bauen (Musik)

```bash
source ~/dev/github/game-audio-forge/scripts/gaf-env
cd ~/dev/github/tools/acestep.cpp
git clone --recurse-submodules https://github.com/ServeurpersoCom/acestep.cpp.git .
mkdir build && cd build
cmake .. -DGGML_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$GAF_CUDA/bin/nvcc" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CUDA_ARCHITECTURES="120a-real"
cmake --build . --config Release -j "$(nproc)"
```

Binaries: `build/{ace-lm,ace-synth,ace-server,ace-understand,neural-codec,mp3-codec,quantize}`.

## 5. audiogen.cpp bauen (SFX)

```bash
source ~/dev/github/game-audio-forge/scripts/gaf-env
cd ~/dev/github/tools/audiogen.cpp
git clone --recurse-submodules https://github.com/audiohacking/audiogen.cpp.git .
cmake -B build-cuda -S . -DCMAKE_BUILD_TYPE=Release \
  -DDASHENG_AUDIOGEN_METAL=OFF -DDASHENG_AUDIOGEN_CUDA=ON \
  -DCMAKE_CUDA_COMPILER="$GAF_CUDA/bin/nvcc"
cmake --build build-cuda --parallel
```

Binary: `build-cuda/dasheng-audiogen`.

> audiogen.cpp nutzt ggml 0.15.2, das `121a` korrekt hinter CUDA ≥ 12.9
> gated — mit 12.8 wird nur `120a-real` gebaut. Kein Arch-Override nötig.

## 6. Modelle laden

```bash
# Musik: ACE-Step 1.5, 0.6B LM + Turbo DiT, Q8_0 (~4.1 GB)
cd ~/dev/github/tools/acestep.cpp
./models.sh --lm 0.6B --quant Q8_0

# SFX: Dasheng AudioGen Q4 (~2.8 GB)
cd ~/dev/github/tools/audiogen.cpp
make download-models-q4
```

## 7. Verifizieren

```bash
gaf status
```

## VRAM-Budget (8 GB)

| Generator | Modelle | VRAM (Gewichte) |
|---|---|---|
| acestep.cpp | 0.6B LM Q8_0 (677 MB) + Turbo DiT Q8_0 (2.4 GB) + Text-Enc Q8_0 (748 MB) + VAE BF16 (322 MB) | ~4.1 GB |
| audiogen.cpp | T5 F32 (1.36 GB) + DiT Q4_0 (1.2 GB) + Vocoder F16 (332 MB) | ~3 GB |

Beide passen einzeln in 8 GB. **Nicht parallel** auf der GPU laufen lassen
(`gaf` wartet, bis die GPU frei ist).
