# Status — Milestone 1 (Foundation)

**Date:** 2026-11-04
**State:** M1 complete. Pushed to a private GitHub repo; milestones + tracking
issues created. No GitHub Actions CI (by design).

## What is in place

- Local git repo at `~/dev/github/game-audio-forge`, branch `main`, pushed to
  `dcschmid/game-audio-forge` (private). Commits are SSH-signed.
- Full directory scaffold: `src/ config/ scripts/ tests/ docs/ presets/
  examples/`.
- Core docs: `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
  `CONTRIBUTING.md`, `CHANGELOG.md`.
- `docs/`: `DATA-LAYOUT.md` (XDG contract), `DEPENDENCIES.md` (license
  inventory + integration decision), `DEVELOPMENT.md`.
- `config/gaf.toml` — shipped default config (engine, backends, model
  registry, profiles, export, processing).
- Example presets (`presets/sfx/door-slam.toml`,
  `presets/music/cave-ambience-loop.toml`) and example profile
  (`examples/profiles/demo.toml`).
- `.gitignore` + `.editorconfig`.
- `LICENSE` — Apache License 2.0 (project license decided 2026-11-04).
- **No GitHub Actions CI** — tests and checks run locally (by design).

## Key architectural decision

**audio.cpp (Apache-2.0, C++/ggml) is the inference foundation.** It natively
wraps every model this project targets — ACE-Step 1.5 (music),
MiDashengLM-Gen (SFX), Stable Audio 3.0 (SFX+Music), ControlFoley (SFX),
MiniMax Music 3 — so one engine covers both the music and SFX paths. Game
Audio Forge is a thin native C++ layer over it (library, profiles, presets,
QC, export, MCP).

**Soundbox (MIT) is a product-shape reference, not a dependency** — it is a
Node/Electron app, not a C++ library. We borrow its ideas (profiles,
license-before-download, MCP surface) and build our own native layer.

**ACE-Step-1.5 / MiDashengLM-Gen** Python repos are reference-only; we use
their GGUF weights (runtime download), not their code. **FFmpeg** is a system
dependency (LGPL, dynamic link).

## Open decisions (need user input)

1. **In-process vs server** integration with audio.cpp — to be decided in M2
   after a build test.
2. **Open-source publication** — deferred until the dependency/model-weight
   review is complete. (Project license is set: **Apache-2.0**.)

## GitHub state

- Private repo `dcschmid/game-audio-forge` created and pushed (branch `main`).
- Description set; visibility **private**.
- Milestones M1–M5 created; tracking issues #1–#5 (one per milestone) plus
  concrete M2 sub-issues #6–#9. M1 tracking issue (#1) is closed.
- Hygiene verified: no model weights, no audio, no home paths tracked; commits
  are SSH-signed.

## Recommended next steps

**Milestone 2 — Audio Engines (the real work):**
1. Build audio.cpp with CUDA + composite model set
   (`ace_step,midashenglm_gen,stable_audio,controlfoley`).
2. Add the first C++ source in `src/` + `CMakeLists.txt` (FetchContent for
   audio.cpp, system FFmpeg).
3. Generate one music take (ACE-Step 1.5) and one SFX take (MiDashengLM-Gen)
   over the C ABI; measure RTF + peak VRAM on the RTX 5070 Laptop (8 GB).
4. Implement the model registry + on-demand download + license display.
5. Decide in-process vs server integration.

**Before M2:** the project license is set (Apache-2.0). The remaining
pre-M2 item is the dependency/model-weight review that gates open-source
publication.
