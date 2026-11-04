# Status — Milestone 1 (Foundation)

**Date:** 2026-11-04
**State:** M1 complete locally. Remote push pending GitHub auth.

## What is in place

- Local git repo at `~/dev/github/game-audio-forge`, branch `main`, one
  SSH-signed commit.
- Full directory scaffold: `src/ config/ scripts/ tests/ docs/ presets/
  examples/ .github/workflows/`.
- Core docs: `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`,
  `CONTRIBUTING.md`, `CHANGELOG.md`.
- `docs/`: `DATA-LAYOUT.md` (XDG contract), `DEPENDENCIES.md` (license
  inventory + integration decision), `DEVELOPMENT.md`.
- `config/gaf.toml` — shipped default config (engine, backends, model
  registry, profiles, export, processing).
- Example presets (`presets/sfx/door-slam.toml`,
  `presets/music/cave-ambience-loop.toml`) and example profile
  (`examples/profiles/demo.toml`).
- CI (`.github/workflows/ci.yml`): config validation, secret scan (gitleaks),
  repo hygiene (no weights/audio/home-paths), test skeleton.
- `.gitignore` + `.editorconfig`.
- `LICENSE` — Apache License 2.0 (project license decided 2026-11-04).

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

- `gh` CLI is **not authenticated** (no `hosts.yml`, no token, no netrc).
- Per the task condition, the private remote repo was **not** created and the
  commit was **not** pushed.
- `origin` is pre-set to `git@github.com:dcschmid/game-audio-forge.git`.
- Local hygiene verified: no model weights, no audio, no home paths tracked;
  commit is SSH-signed.

## Recommended next steps

**Immediate (unblock GitHub):**
1. Authenticate: `gh auth login` (or set `GH_TOKEN`).
2. Create the private repo and push:
   ```bash
   gh repo create dcschmid/game-audio-forge --private \
     --description "Local-first, C++-powered AI audio production toolkit for game sound effects, instrumental music, sound libraries and engine-ready exports."
   git push -u origin main
   ```
3. Create milestones M1–M5 and the tracking issues (see `ROADMAP.md`).

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
