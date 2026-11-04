# Development

How to set up the development environment and build/test the project.

## Target platform

- **Fedora Workstation 44**
- Framework Laptop 16
- **NVIDIA RTX 5070 Laptop GPU, 8 GB VRAM** (CUDA preferred)

The architecture keeps Vulkan, ROCm and CPU backends open, but the primary
development and benchmarking target is CUDA on the RTX 5070 Laptop.

## Prerequisites

| Tool | Version | Notes |
|---|---|---|
| GCC | 13+ | C++20 (our layer) |
| CMake | 3.25+ | build system (our layer) |
| Ninja | any | generator |
| FFmpeg | 6.x | system package (`dnf install ffmpeg`), LGPL |
| git | 2.x | |
| NVIDIA driver | 615+ | for the CUDA backend (the engine binary bundles the CUDA runtime) |

Install on Fedora:

```bash
sudo dnf install gcc gcc-c++ cmake ninja-build ffmpeg git
```

> **No CUDA toolkit is required.** The audio.cpp engine is used as a prebuilt
> CUDA binary (it bundles `libcublas`/`libcudart`), so only the NVIDIA *driver*
> is needed. A from-source build of audio.cpp (for a C-ABI integration) would
> need the CUDA toolkit — see [ARCHITECTURE.md](ARCHITECTURE.md) for the
> integration decision.

## The audio.cpp engine (runtime dependency)

The engine is a prebuilt binary, kept in `.tools/` (gitignored — never
committed). It runs as a local HTTP server that our CLI talks to.

```bash
# 1. Download the prebuilt CUDA binary + CUDA runtime (one-time, ~1 GB)
cd .tools
gh release download v0.9.3 -R 0xShug0/audio.cpp \
  --pattern "audio-v0.9.3-bin-ubuntu-x64-cuda12.8-colab.tar.gz" --dir .
gh release download v0.9.3 -R 0xShug0/audio.cpp \
  --pattern "audio-v0.9.3-cudart-ubuntu-x64-cuda12.8.tar.gz" --dir .
tar xzf audio-v0.9.3-bin-ubuntu-x64-cuda12.8-colab.tar.gz
tar xzf audio-v0.9.3-cudart-ubuntu-x64-cuda12.8.tar.gz

# 2. Start the server (CUDA backend, model management on)
#    libnccl.so.2 is needed at load; point LD_LIBRARY_PATH at it (e.g. from a
#    local nvidia-nccl install) plus the bundled CUDA libs.
LD_LIBRARY_PATH=.:/path/to/nvidia/nccl/lib \
  ./audiocpp_server --config ./server.json --ui-management --no-ui \
  --model-spec-override ./model_specs
```

`server.json` registers the installed models (see the M2 setup). The server
listens on `127.0.0.1:17495`.

## Build our layer (the `gaf` CLI)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build -j
ctest --test-dir build --output-on-failure
```

The only third-party C++ dependency is `nlohmann/json`, fetched at configure
time (FetchContent) — not committed. The engine is a runtime dependency
(reached over HTTP), not a link dependency.

## Configuration

- Shipped default: [`config/gaf.json`](../config/gaf.json).
- Seed the user config: `gaf config init` (copies the shipped default to
  `~/.config/game-audio-forge/gaf.json`).
- Machine-specific overrides: `~/.config/game-audio-forge/gaf.local.json`
  (gitignored).

See [DATA-LAYOUT.md](DATA-LAYOUT.md) for the full path-resolution contract.

## Models

Model weights are **not** in the repository. They are downloaded on demand into
the XDG data directory (`~/.local/share/game-audio-forge/models`) via the
engine's model-management endpoint:

```bash
# Install a package (e.g. the default ACE-Step 1.5 Turbo Q8_0)
curl -X POST http://127.0.0.1:17495/v1/ui/models/install \
  -H 'Content-Type: application/json' -d '{"id":"ace_step_turbo_q8_0"}'
curl -X POST http://127.0.0.1:17495/v1/ui/models/install \
  -H 'Content-Type: application/json' -d '{"id":"midashenglm_gen_q8_0"}'
```

The license for each model is recorded per take. See [BENCHMARKS.md](BENCHMARKS.md)
for what fits the 8 GB budget.

## Using the CLI

```bash
gaf config show                 # resolved config + paths
gaf models list                 # models registered with the engine
gaf generate midashenglm_gen --text "door slam" --duration 3 --seed 42
gaf generate ace_step --text "dark ambient loop" --duration 24 --seed 7
gaf library list --profile default
scripts/gaf-benchmark.sh ace_step "dark ambient loop" 24 7
```

## Testing

- **Unit tests** (no GPU): config resolution (XDG paths, JSON parse) and the
  library (add/find, round-trip, empty). Run locally via
  `ctest --test-dir build`.
- **Integration** (GPU + model): generation is exercised manually via the CLI
  and `scripts/gaf-benchmark.sh`; an automated round-trip test is an M3 item.
- **Benchmarks**: RTF and peak VRAM on the RTX 5070 Laptop — see
  [BENCHMARKS.md](BENCHMARKS.md).

No GitHub Actions CI is configured (by design). Tests and checks run locally.

## Do / Don't

- **Do** keep the product layer thin; inference belongs to audio.cpp.
- **Do** keep backend selection runtime-configurable.
- **Don't** commit model weights, audio, or secrets.
- **Don't** add Python as a runtime or build dependency.
- **Don't** hard-code CUDA.
