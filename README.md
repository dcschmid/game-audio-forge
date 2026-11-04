# Game Audio Forge

Local-first, C++-powered AI audio production toolkit for game sound effects,
instrumental music, sound libraries and engine-ready exports.

> **Status:** Milestone 2 — Audio Engines. The C++ layer (`gaf` CLI) builds
> and runs; ACE-Step 1.5 (music) and MiDashengLM-Gen (SFX) generate locally on
> CUDA, benchmarked on an 8 GB RTX 5070 Laptop. No UI yet. See
> [ROADMAP.md](ROADMAP.md) and [docs/BENCHMARKS.md](docs/BENCHMARKS.md).

## What it is

Game Audio Forge is a standalone application (not an extension of any game)
that runs local AI audio models on your own GPU and turns them into
production-ready game audio:

- **Local AI sound-effect generation** — describe a sound, get a take.
- **Local instrumental music production** — style prompt in, loop or track out.
- **C++-first architecture** — native inference, no mandatory Python runtime.
- **CUDA-first, backend-open** — NVIDIA CUDA preferred; Vulkan, ROCm and CPU
  backends stay first-class options.
- **One central sound & music library** — every take stored with its prompt,
  model, seed and license.
- **Independent project profiles** — one library per game or client.
- **Reusable prompts & presets** — save and share generation recipes.
- **Analysis & post-processing** — denoise, normalize, loudness, mastering basics.
- **Loop creation & variant management** — extend, cover, repaint, split stems.
- **Engine-agnostic exports** — Defold first, more engines later.
- **MCP integration** — drive it from Claude Code and Codex.

## Why C++ / audio.cpp

The inference foundation is
[audio.cpp](https://github.com/0xShug0/audio.cpp) (Apache-2.0), a pure C++/ggml
runtime that already natively wraps the models this project targets:

| Model | Task | License | Fits 8 GB VRAM |
|---|---|---|---|
| ACE-Step 1.5 | instrumental music | MIT | ✅ |
| MiDashengLM-Gen | sound effects / ambience | Apache-2.0 | ✅ |
| Stable Audio 3.0 (SFX + Music) | SFX, loops | Stability AI Community | ✅ |
| ControlFoley | foley SFX | TBD | ✅ |
| MiniMax Music 3 | songs w/ lyrics | MiniMax Community | ❌ (~10 GB) |

audio.cpp gives us CUDA/HIP/Vulkan/Metal/CPU backends, a CMake build with
composite model sets, a native model manager, and a C ABI + server API — so
Game Audio Forge stays a thin, native layer over a proven engine instead of
re-implementing inference.

**Soundbox** (MIT, TypeScript/Electron) is *not* a dependency. It is a
product-shape reference: a UI + HTTP API + 24-tool MCP server that itself wraps
audio.cpp. We borrow its ideas (profiles, license-before-download, MCP surface)
and build our own native layer. See [ARCHITECTURE.md](ARCHITECTURE.md).

## Repository layout

```
game-audio-forge/
├── src/            # our C++ code (thin layer over the audio.cpp engine)
│   ├── gaf/        #   core library: config, http client, library
│   └── gaf_cli.cpp #   the `gaf` CLI
├── config/         # shipped default configuration (gaf.json)
├── presets/        # reusable prompt & generation presets
├── examples/       # example project profiles & metadata
├── scripts/        # benchmark + helper scripts
├── tests/          # unit tests
├── docs/           # architecture, data layout, dependencies, benchmarks
├── CMakeLists.txt  # builds the `gaf` CLI + tests
├── README.md
├── ARCHITECTURE.md
├── ROADMAP.md
├── CONTRIBUTING.md
└── CHANGELOG.md
```

Large assets — model weights, GGUF files, sound libraries, generated audio,
WAV/FLAC masters, temp inference data — live **outside** the repository in an
XDG-compatible data directory. The prebuilt engine binary lives in `.tools/`
(gitignored). See [docs/DATA-LAYOUT.md](docs/DATA-LAYOUT.md).

## Getting started

The `gaf` CLI is a thin client over the audio.cpp engine (a local HTTP server).
Full setup is in [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md); in short:

- Fedora 44 (or any recent Linux), GCC 13+, CMake, Ninja, FFmpeg
- NVIDIA driver (the engine binary bundles the CUDA runtime — no toolkit needed)

```bash
# 1. Set up the engine (prebuilt CUDA binary + server) — see DEVELOPMENT.md
# 2. Build our layer
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build -j
ctest --test-dir build --output-on-failure

# 3. Use it
./build/gaf config init
./build/gaf generate midashenglm_gen --text "door slam" --duration 3 --seed 42
./build/gaf generate ace_step --text "dark ambient loop" --duration 24 --seed 7
```

## Data & privacy

- No model weights or audio files are committed.
- No telemetry, no accounts, no network calls except explicit model downloads.
- Everything stays on your machine.

## License

**Apache License 2.0** — see [LICENSE](LICENSE).

The project is currently **private**; open-source publication is deferred until
the dependency and model-weight review is complete. The Apache-2.0 choice
matches the primary dependency (audio.cpp) and includes a patent grant. Model
weights keep their own licenses and are not redistributed by this project. See
[docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) for the full license inventory.

## Roadmap

- **M1 — Foundation** ✅: repo, architecture, dev environment, dependency
  strategy, license (Apache-2.0), docs.
- **M2 — Audio Engines** ✅: audio.cpp engine (server), ACE-Step 1.5 +
  MiDashengLM-Gen generate on CUDA, GPU benchmarks, model management, `gaf`
  CLI + CMake + unit tests.
- **M3 — Audio Workspace**: project profiles, sound library, prompt management,
  presets, generation + QC.
- **M4 — Processing & Export**: analysis, mastering basics, loops, variants,
  Defold export.
- **M5 — Automation**: MCP, batch generation, automated tests, agent workflows.

Full detail in [ROADMAP.md](ROADMAP.md).
