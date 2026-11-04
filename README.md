# Game Audio Forge

Local-first, C++-powered AI audio production toolkit for game sound effects,
instrumental music, sound libraries and engine-ready exports.

> **Status:** Milestone 1 — Foundation. The repository, architecture,
> dependency strategy and data layout are in place. No models are installed
> and no UI is implemented yet. See [ROADMAP.md](ROADMAP.md).

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
├── src/            # our C++ code (thin layer over audio.cpp)
├── config/         # shipped default configuration (gaf.toml)
├── presets/        # reusable prompt & generation presets
├── examples/       # example project profiles & metadata
├── scripts/        # build, model-download, benchmark, export helpers
├── tests/          # unit + integration tests
├── docs/           # architecture, data layout, dependency & license notes
├── .github/        # CI workflows
├── README.md
├── ARCHITECTURE.md
├── ROADMAP.md
├── CONTRIBUTING.md
└── CHANGELOG.md
```

Large assets — model weights, GGUF files, sound libraries, generated audio,
WAV/FLAC masters, temp inference data — live **outside** the repository in an
XDG-compatible data directory. See [docs/DATA-LAYOUT.md](docs/DATA-LAYOUT.md).

## Getting started

Milestone 1 ships configuration and documentation only. To build the C++ layer
you will need (see [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)):

- Fedora 44 (or any recent Linux), GCC 13+, CMake, Ninja
- NVIDIA CUDA Toolkit (for the preferred backend)
- FFmpeg (system package, LGPL)

```bash
# 1. Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAUDIOCPP_BACKEND=cuda
# 2. Build
cmake --build build -j
# 3. Test
ctest --test-dir build
```

> The CMake build is added in Milestone 2 once the first C++ source lands.
> Until then the repository is configuration + documentation.

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

- **M1 — Foundation** (this commit): repo, architecture, dev environment,
  dependency strategy, license concept, docs.
- **M2 — Audio Engines**: evaluate audio.cpp, test ACE-Step + SFX models over
  C++, GPU benchmarks, local model management.
- **M3 — Audio Workspace**: project profiles, sound library, prompt management,
  presets, generation + QC.
- **M4 — Processing & Export**: analysis, mastering basics, loops, variants,
  Defold export.
- **M5 — Automation**: MCP, batch generation, automated tests, agent workflows.

Full detail in [ROADMAP.md](ROADMAP.md).
