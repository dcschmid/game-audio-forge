# Changelog

All notable changes to Game Audio Forge are documented here.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- **Milestone 2 — Audio Engines.**
  - C++ layer: `gaf` CLI + core library (config, HTTP client, library) in
    `src/`; CMake build (C++20, Ninja).
  - Engine integration via the audio.cpp server (prebuilt CUDA binary); the
    CLI is a thin HTTP client.
  - `gaf` commands: `config init|show`, `models list|status`,
    `generate <model>`, `library list`.
  - `scripts/gaf-benchmark.sh` — RTF + peak-VRAM benchmark.
  - `docs/BENCHMARKS.md` — measured RTF/VRAM for ACE-Step 1.5 and
    MiDashengLM-Gen on an 8 GB RTX 5070 Laptop.
  - Unit tests (7): config (XDG paths, JSON) and library (add/find, round-trip).
- **Milestone 1 — Foundation:** repository, architecture, dependency strategy,
  data layout, default configuration, and documentation.
- `LICENSE` — Apache License 2.0 (project license decided 2026-11-04).
- `config/gaf.json` — shipped default configuration (engine, backends, model
  registry, profiles, export, processing).
- `docs/` — ARCHITECTURE, DATA-LAYOUT, DEPENDENCIES, DEVELOPMENT, BENCHMARKS.
- GitHub milestones M1–M5 and tracking issues.

### Changed
- Configuration format is **JSON** (`config/gaf.json`); presets and example
  profiles are JSON too (M1 shipped TOML).
- Integration decision recorded: **server** (not in-process C ABI).

### Notes
- No model weights or audio files are committed; the engine binary lives in
  `.tools/` (gitignored).
- Project license is **Apache-2.0**. Open-source publication is deferred until
  the dependency and model-weight review is complete.
- No GitHub Actions CI is configured (by design); tests and checks run locally.

## [0.1.0] - 2026-11-04

### Added
- Initial project scaffold (Milestone 1 — Foundation).
