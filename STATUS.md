# Status — Milestone 3 (Audio Workspace)

**Date:** 2026-11-04
**State:** M3 complete. The `gaf` CLI now manages project profiles, reusable
presets, and a per-profile library with automatic quality control, and
orchestrates multi-take generation. All 18 unit + integration tests pass,
including a live generation round-trip against the audio.cpp server.

## What is in place

- **C++ layer** (`src/`): `gaf` CLI + core library. Modules:
  - `config` — XDG path resolution, JSON config, model registry
  - `http` — minimal POSIX-socket HTTP/1.1 client (no TLS, local server)
  - `library` — per-profile JSON take store (add/find/remove, QC fields)
  - `profiles` — independent project scopes + active-profile state
  - `presets` — reusable generation recipes (shipped + user, shadowing)
  - `qc` — dependency-free WAV analyzer (duration, peak, RMS, silence, LUFS)
  CMake build (C++20, Ninja). Only third-party dep is `nlohmann/json`
  (FetchContent, not committed).
- **Engine integration: server.** audio.cpp v0.9.3 runs as a local HTTP server
  (prebuilt CUDA 12.8 binary in `.tools/`, gitignored). The CLI is a thin
  client over `/v1/tasks/run`, `/v1/models`, and model-management endpoints.
- **Models installed** (XDG data dir, not in the repo):
  - ACE-Step 1.5 Turbo Q8_0 (6.19 GB) — music
  - MiDashengLM-Gen Q8_0 (3.38 GB) — SFX

## CLI surface (M3)

```
gaf config init|show
gaf models list|status
gaf profile create|list|show|use|rm
gaf preset  list|show|save|rm
gaf generate <model|preset> [--text P] [--duration N] [--seed N]
                    [--count N] [--tags a,b] [--profile NAME] [--out FILE] [--no-qc]
gaf library list [--profile NAME] [--kind K] [--all]
gaf library show <id> [--profile NAME]
gaf library rm <id> [--profile NAME]
gaf library import <file> [--profile NAME] [--kind K] [--prompt P] [--tags a,b]
```

- **Profiles** are independent library scopes + settings (sample rate, bit
  depth, loudness target, export target). The active profile is persisted in
  the config dir (`state.json`); `gaf generate` writes to it by default.
- **Presets** are reusable recipes (model, prompt, duration, seed, tags, kind).
  Shipped presets live in the repo (`presets/`); user presets live in the XDG
  data dir and shadow shipped ones of the same name. `gaf generate <preset>`
  loads and runs one.
- **Quality control** runs automatically after every generation/import: the
  WAV is parsed and duration, peak, RMS, an approximate LUFS, and head/tail
  silence are stored on the take. A take is flagged `QC!` if it is silent or
  over-loud.
- **Orchestration:** `--count N` produces N retakes (seed incremented per
  retake); `--tags` and `--profile` are recorded on each take.

## Tests

18 passing:
- config (XDG paths, env override, JSON load, missing file)
- library (add/find, round-trip, empty)
- qc (sine amplitude/duration, digital silence, head/tail silence, invalid file)
- profiles (create/get, list/remove, active persistence)
- presets (save/find, shipped/user shadowing, remove)
- **generate round-trip** (integration): POSTs a short SFX to the live engine,
  decodes the base64 WAV, and verifies it parses and is non-silent. Skips
  cleanly when no engine is reachable.

## Key decisions

- **Server, not in-process C ABI** (carried from M2). The prebuilt CUDA binary
  runs without installing the CUDA toolkit, gives process isolation, and serves
  a future UI + MCP unchanged.
- **JSON library, not SQLite, for M3.** The per-profile JSON store is stable
  and dependency-free; the API is small enough that a later migration to
  SQLite (for a cross-profile "central" index) is cheap. Documented in
  `docs/DATA-LAYOUT.md`.
- **QC is pure C++ (no ffmpeg).** Peak/RMS/silence/duration come from parsing
  the PCM WAV directly. LUFS is an approximation (RMS-based); true ITU-R BS.1770
  loudness is an M4 (mastering) task.

## Known limitations / M4+ work

- **Duration is a budget, not a cap.** ACE-Step's planner chose 118 s for an 8 s
  request; MiDashengLM-Gen produced 4–5 s for a 2 s request. Exact-length loop
  trimming is an M4 (Processing) task.
- **Model-swap under `max_loaded_models: 1`.** Rapidly alternating models in one
  batch can hit a transient engine error while the resident model is swapped;
  the CLI continues past it. A single-model batch is reliable.
- **Stable Audio 3.0 / ControlFoley** not yet benchmarked (next models to add).
- **License display** is recorded per take but not yet shown *before* download
  in the CLI (the engine shows it in its WebUI).

## Open decisions

1. **Open-source publication** — deferred until the dependency/model-weight
   review is complete. (Project license is set: Apache-2.0.)

## Next: Milestone 4 — Processing & Export

- Audio analysis (spectral, onset, loop-point detection)
- Mastering basics (normalize, loudness target via true LUFS, denoise)
- Music loops (exact-length trimming, crossfade)
- Variant management (A/B takes, selection)
- Defold export as the first engine profile
- Further engines later
