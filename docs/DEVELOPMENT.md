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
| GCC | 13+ | C++20 |
| CMake | 3.25+ | build system |
| Ninja | any | generator |
| CUDA Toolkit | 12.x | for the CUDA backend |
| FFmpeg | 6.x | system package (`dnf install ffmpeg`), LGPL |
| git | 2.x | |

Install on Fedora:

```bash
sudo dnf install gcc gcc-c++ cmake ninja-build ffmpeg git
# CUDA Toolkit: see NVIDIA's Fedora instructions, or use the system cuda package
```

## Build

> The CMake build is added in **Milestone 2** when the first C++ source lands.
> Until then the repository is configuration + documentation and there is
> nothing to compile. The commands below are the intended workflow.

```bash
# Configure (CUDA backend, composite model set)
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -GNinja \
  -DAUDIOCPP_BACKEND=cuda

# Build
cmake --build build -j

# Run tests
ctest --test-dir build --output-on-failure
```

### Backend selection

- Build-time: `-DAUDIOCPP_BACKEND=cuda|vulkan|rocm|cpu`.
- Runtime: the `engine.backends` preference list in `config/gaf.toml` chooses
  the first available backend at startup.

### Composite model set

Only the model families we use are built, keeping the binary small:

```
-DAUDIOCPP_MODEL_SET=custom
-DAUDIOCPP_MODELS=ace_step,midashenglm_gen,stable_audio,controlfoley
```

## Configuration

- Shipped default: [`config/gaf.toml`](../config/gaf.toml).
- User config: copy it to `~/.config/game-audio-forge/gaf.toml` and edit.
- Machine-specific overrides: `~/.config/game-audio-forge/gaf.local.toml`
  (gitignored).

See [DATA-LAYOUT.md](DATA-LAYOUT.md) for the full path-resolution contract.

## Models

Model weights are **not** in the repository. They are downloaded on demand into
the XDG data directory via the audio.cpp model manager:

```bash
# (M2) example — download the default ACE-Step 1.5 package
gaf model install ace_step_1_5_turbo
```

The license for each model is shown before download and recorded per take.

## Testing

- **Unit tests** (no GPU): config resolution, library, profiles, presets,
  export mapping. Must pass in CI.
- **Integration tests** (GPU + model): generation round-trip. Skipped when no
  GPU or no model is present; clearly marked.
- **Benchmarks** (M2): RTF and peak VRAM on the RTX 5070 Laptop for each model
  and quantization.

## CI

GitHub Actions (`.github/workflows/ci.yml`):

- Lint (config + code style).
- Secret scan (gitleaks) — fail on tokens/credentials/paths.
- Unit tests on CPU (no GPU in the default runner).
- GPU jobs are opt-in (self-hosted or a GPU runner) for M2 benchmarks.

## Do / Don't

- **Do** keep the product layer thin; inference belongs to audio.cpp.
- **Do** keep backend selection runtime-configurable.
- **Don't** commit model weights, audio, or secrets.
- **Don't** add Python as a runtime or build dependency.
- **Don't** hard-code CUDA.
