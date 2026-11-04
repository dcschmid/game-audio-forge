# Architecture

Game Audio Forge is a **thin, native C++ layer over audio.cpp**, with a
configurable data layout, a central library, project profiles, and an
engine-agnostic export path. This document is the source of truth for how the
pieces fit together.

## Design principles

1. **C++-first, no mandatory Python.** Inference runs in-process through
   audio.cpp (C++/ggml). Python is allowed only for optional reference
   tooling and is never a runtime dependency.
2. **CUDA-first, backend-open.** The default backend is NVIDIA CUDA. The
   architecture must keep Vulkan, ROCm/HIP and CPU as first-class options
   (audio.cpp already provides all of them; we must not hard-code CUDA).
3. **Thin over a proven engine.** We do not re-implement model inference.
   audio.cpp owns model loading, ggml backends, and the C ABI / server API.
   Our code owns the product: library, profiles, presets, QC, export, MCP.
4. **Local-first, private by default.** No telemetry, no accounts. Network
   access is limited to explicit model-weight downloads.
5. **Assets never in the repo.** Weights, generated audio and masters live in
   an XDG data directory, not in git.
6. **Engine-agnostic output.** Exports target a neutral intermediate (WAV/FLAC
   + metadata) and are then mapped to engine-specific formats (Defold first).

## Layered view

```
┌──────────────────────────────────────────────────────────────────┐
│  Agent / UI layer                                                 │
│  • MCP server (Claude Code, Codex)                                │
│  • CLI (gaf)                                                      │
│  • (later) native UI — product shape informed by Soundbox         │
├──────────────────────────────────────────────────────────────────┤
│  Product layer (our C++ code, src/)                               │
│  • Project profiles        • Sound & music library                │
│  • Prompt & preset mgmt    • Generation orchestration + QC        │
│  • Analysis & post-process • Loop/variant management              │
│  • Engine-agnostic export  • Model registry & download            │
├──────────────────────────────────────────────────────────────────┤
│  Inference foundation (audio.cpp, Apache-2.0)                     │
│  • ggml runtime: CUDA / Vulkan / ROCm / Metal / CPU               │
│  • Model families: ace_step, midashenglm_gen, stable_audio,       │
│    controlfoley, minimax_music3, …                                │
│  • C ABI + server API + native model manager                      │
├──────────────────────────────────────────────────────────────────┤
│  System dependencies                                              │
│  • FFmpeg (LGPL, dynamic) — decode/encode, resample, loudness     │
│  • CUDA Toolkit / Vulkan SDK / ROCm — per backend                 │
└──────────────────────────────────────────────────────────────────┘
```

## Components

### 1. Inference foundation — audio.cpp

- **Role:** all model inference. We do not fork or copy audio.cpp's source.
- **Integration (decided in M2): server.** audio.cpp runs as a local HTTP
  server (`audiocpp_server`); our C++ layer is a thin client over its
  `/v1/tasks/run`, `/v1/models`, and model-management endpoints. Rationale:
  - The prebuilt CUDA binary runs on the target hardware without installing the
    CUDA toolkit (no `sudo` needed), which a from-source C-ABI build would
    require.
  - Process isolation: a model OOM/crash does not take down the CLI.
  - The same server serves a future UI and the MCP layer (M5) unchanged.
  - *In-process (C ABI)* remains the option for lower latency once a CUDA
    toolkit is available for a from-source build; the client interface is the
    same either way.
- **Model set:** only the families we use are registered
  (`ace_step`, `midashenglm_gen`, + `stable_audio`/`controlfoley` later),
  keeping the working set small.
- **Backends:** selected at server start (`--backend cuda`) and/or via the
  `engine.backends` preference list in `config/gaf.json`.
- **8 GB VRAM budget:** `max_loaded_models: 1` (one model resident at a time)
  and `mem_saver: true` for ACE-Step (releases staged state between phases)
  so the DiT + planner + VAE fit. See `docs/BENCHMARKS.md`.

### 2. Product layer — our C++ code (`src/`)

This is the code we own. It is deliberately small and sits on top of
audio.cpp:

- **`library`** — the central sound & music library. Each take is a record:
  audio file (in the data dir) + metadata (prompt, model, seed, duration,
  sample rate, license, tags, profile). Backed by a single SQLite file.
- **`profiles`** — independent project profiles. A profile is a named library
  scope + settings (e.g. one per game). Profiles share the model cache.
- **`prompts` / `presets`** — reusable generation recipes. Shipped presets live
  in `/presets`; user presets live in the data dir.
- **`generate`** — orchestration: resolve model + preset → call audio.cpp →
  write take → register in library → run QC.
- **`qc`** — quality control: duration, loudness, silence detection, optional
  denoise/normalize.
- **`analysis`** — spectral/loudness analysis for the library and for export.
- **`loops` / `variants`** — loop creation, extend/cover/repaint, stem split,
  variant management.
- **`export`** — engine-agnostic export. Neutral intermediate (WAV/FLAC +
  sidecar metadata) → engine-specific mapping. Defold is the first target.
- **`models`** — model registry + on-demand download via the audio.cpp model
  manager, with license shown before download.
- **`mcp`** — MCP server exposing the product layer to Claude Code / Codex.

### 3. Agent / UI layer

- **MCP server** (M5): the primary programmatic surface. Tools mirror the
  product layer (generate, library query, export, profile switch, …).
- **CLI** (`gaf`): the primary human surface from M2 onward.
- **Native UI** (later): optional. Soundbox is the product-shape reference
  (timeline, library panel, license-before-download, profiles in the header).
  We do **not** embed Soundbox; we build our own if/when a UI is wanted.

## Why not embed Soundbox?

Soundbox is a complete TypeScript/Electron application that *already* wraps
audio.cpp and ships a UI + HTTP API + MCP server. It is a strong reference, but
it is not a C++ foundation:

- It is a Node/Electron app, not a C++ library — it cannot be the "C++-first"
  core this project requires.
- Embedding it would mean depending on a whole second product (its UI, its
  storage, its model catalog) rather than owning a thin native layer.
- Its storage (`~/.soundbox`, SQLite + WAV folder) and profile model are useful
  *ideas* we adopt, but we implement them natively.

**Decision:** treat Soundbox as a reference implementation and design
inspiration. Do not vendor or depend on it. Re-evaluate only if we decide the
product is a GUI app rather than a native toolkit (a separate decision).

## Data layout (XDG)

All large data lives outside the repo, under the XDG data home:

```
$XDG_DATA_HOME/game-audio-forge/        (default: ~/.local/share/game-audio-forge)
├── models/        # GGUF / safetensors weights (downloaded)
├── libraries/     # generated + imported audio, per profile
│   └── <profile>/
├── projects/      # project profile definitions
├── presets/       # user presets
└── cache/         # temp inference data

$XDG_CONFIG_HOME/game-audio-forge/      (default: ~/.config/game-audio-forge)
└── gaf.json       # user config (copy of config/gaf.json + overrides)
                   # gaf.local.json     # machine-specific, gitignored

$XDG_CACHE_HOME/game-audio-forge/       (default: ~/.cache/game-audio-forge)
└── ...            # scratch, model download cache
```

Full contract in [DATA-LAYOUT.md](DATA-LAYOUT.md).

## Build & test strategy

- **CMake** is used *only* because we own C++ code (the product layer). It
  fetches audio.cpp as a third-party dependency (FetchContent / subproject) and
  links FFmpeg from the system.
- **Composite model set** keeps the build to the families we use.
- **Tests:**
  - Unit tests (C++): library, profiles, presets, export mapping, config
    resolution. No GPU required.
  - Integration tests: generation round-trip (skipped when no GPU / no model).
  - Run locally via `ctest`. No GitHub Actions CI is configured (by design).
- **No Python in the build.** Python is optional reference tooling only.

## Hardware target

- Fedora 44, Framework Laptop 16, **RTX 5070 Laptop GPU, 8 GB VRAM**.
- CUDA is the preferred backend. The 8 GB budget fits ACE-Step 1.5,
  MiDashengLM-Gen and Stable Audio 3.0 (SFX/Music) at sensible quantizations;
  MiniMax Music 3 (~10 GB) is off by default.
- Backend selection stays runtime-configurable so Vulkan/ROCm/CPU work without
  code changes.

## Security & privacy

- No telemetry, no accounts, no phone-home.
- Network access only for explicit model downloads (user-initiated).
- No secrets, tokens or machine-specific paths are committed (enforced by
  `.gitignore` + a pre-commit secret scan in CI).
- Model weights keep their original licenses; the app shows each license before
  download and records it per take.
