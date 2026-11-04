# Status — Milestone 2 (Audio Engines)

**Date:** 2026-11-04
**State:** M2 complete. The `gaf` CLI builds and runs; ACE-Step 1.5 (music)
and MiDashengLM-Gen (SFX) generate locally on CUDA, benchmarked on an 8 GB
RTX 5070 Laptop.

## What is in place

- **C++ layer** (`src/`): `gaf` CLI + core library (config, HTTP client,
  library). CMake build (C++20, Ninja). Only third-party dep is `nlohmann/json`
  (FetchContent, not committed).
- **Engine integration: server.** audio.cpp v0.9.3 runs as a local HTTP server
  (prebuilt CUDA 12.8 binary in `.tools/`, gitignored). Our CLI is a thin
  client over `/v1/tasks/run`, `/v1/models`, and model-management endpoints.
  No CUDA toolkit needed (the binary bundles the CUDA runtime).
- **Models installed** (XDG data dir, not in the repo):
  - ACE-Step 1.5 Turbo Q8_0 (6.19 GB) — music
  - MiDashengLM-Gen Q8_0 (3.38 GB) — SFX
- **CLI surface:** `gaf config init|show`, `gaf models list|status`,
  `gaf generate <model> --text … --duration N --seed N`, `gaf library list`.
- **Benchmarks** ([docs/BENCHMARKS.md](docs/BENCHMARKS.md)):
  - MiDashengLM-Gen: 20 s SFX, RTF 0.24, peak VRAM 3805 MiB
  - ACE-Step 1.5: 205 s music, RTF 0.13, peak VRAM 6947 MiB (needs `mem_saver`)
- **Unit tests:** 7 passing (config XDG/JSON, library add/find/round-trip).
- **Config:** `config/gaf.json` (JSON, not TOML) — engine, backends, model
  registry, profiles, export, processing.

## Key decisions

- **Server, not in-process C ABI.** The prebuilt CUDA binary runs without
  installing the CUDA toolkit (no `sudo`), gives process isolation, and serves
  a future UI + MCP unchanged. In-process remains an option once a CUDA
  toolkit is available.
- **8 GB VRAM budget:** `max_loaded_models: 1` (one model resident) +
  `mem_saver: true` for ACE-Step. Both default models fit at Q8_0; MiniMax
  Music 3 (~10 GB) does not.
- **Config format is JSON** (matches the C++ reader); presets/profiles are JSON
  too. (M1 shipped TOML; converted in M2.)

## Known limitations / M3+ work

- **Duration is a budget, not a cap.** ACE-Step's planner chose 205 s for a 24 s
  request; MiDashengLM-Gen produced 20 s for a 3 s request. Exact-length loop
  trimming is an M4 (Processing) task.
- **No automated generation test** yet (manual + benchmark script). An
  integration round-trip test is an M3 item.
- **Stable Audio 3.0 / ControlFoley** not yet benchmarked (next models to add).
- **License display** is recorded per take but not yet shown *before* download
  in the CLI (the engine shows it in its WebUI).

## Open decisions

1. **Open-source publication** — deferred until the dependency/model-weight
   review is complete. (Project license is set: Apache-2.0.)

## Next: Milestone 3 — Audio Workspace

- Project profiles (independent libraries, shared model cache)
- Central sound & music library (the JSON library is the start; consider SQLite)
- Prompt management (save, tag, reuse) + presets wired into `gaf generate`
- Generation orchestration + quality control
- Automated generation round-trip test
