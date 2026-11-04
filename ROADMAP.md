# Roadmap

Milestones are ordered by dependency. Each lists its deliverables and a
definition of done. The current milestone is **M1 — Foundation**.

## M1 — Foundation ✅ (this commit)

Goal: a solid, reviewable base — repo, architecture, dev environment,
dependency strategy, license concept, documentation.

- [x] GitHub repository (private) + local git repo
- [x] Project architecture ([ARCHITECTURE.md](ARCHITECTURE.md))
- [x] Development environment notes ([docs/DEVELOPMENT.md](docs/DEVELOPMENT.md))
- [x] Dependency strategy ([docs/DEPENDENCIES.md](docs/DEPENDENCIES.md))
- [x] License concept + decision: **Apache-2.0** ([LICENSE](LICENSE),
  [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md))
- [x] Data layout contract ([docs/DATA-LAYOUT.md](docs/DATA-LAYOUT.md))
- [x] Default configuration ([config/gaf.toml](config/gaf.toml))
- [x] Milestones + tracking issues on GitHub
- [x] No GitHub Actions CI (by design — tests run locally)

**Definition of done:** the repository builds a clear mental model; no models
installed, no UI, no secrets committed; a contributor can read the docs and
understand the plan.

## M2 — Audio Engines

Goal: prove the inference foundation on the target hardware.

- [ ] Evaluate audio.cpp: build with CUDA, composite model set
- [ ] Test ACE-Step 1.5 (music) over C++ (in-process C ABI)
- [ ] Test MiDashengLM-Gen (SFX) over C++
- [ ] GPU benchmarks on RTX 5070 Laptop (8 GB): RTF, peak VRAM, quantization
- [ ] Local model management: registry + on-demand download + license display
- [ ] First C++ source in `src/` + CMake build + unit tests
- [ ] Decide in-process vs server integration

**Definition of done:** a CLI command generates a music take and an SFX take
locally on CUDA, with measured VRAM/RTF, and models are managed from config.

## M3 — Audio Workspace

Goal: the product layer around the engines.

- [ ] Project profiles (independent libraries, shared model cache)
- [ ] Central sound & music library (SQLite + metadata)
- [ ] Prompt management (save, tag, reuse)
- [ ] Music & sound presets (shipped + user)
- [ ] Generation orchestration + quality control
- [ ] CLI surface (`gaf`)

**Definition of done:** a user can create a profile, generate takes from
presets, browse the library, and re-run a saved prompt.

## M4 — Processing & Export

Goal: turn raw takes into engine-ready assets.

- [ ] Audio analysis (spectral, loudness, silence)
- [ ] Mastering basics (normalize, loudness target, denoise)
- [ ] Music loops (loop creation, extend/cover/repaint)
- [ ] Variant management (stems, takes, A/B)
- [ ] Engine-agnostic export (neutral intermediate + sidecar metadata)
- [ ] **Defold export as the first engine profile**
- [ ] Additional engines later

**Definition of done:** a generated take is exported to a Defold-ready asset
with correct sample rate/bit depth and metadata, and loops/variants are
manageable.

## M5 — Automation

Goal: make the toolkit agent- and batch-friendly.

- [ ] MCP server (Claude Code, Codex)
- [ ] Batch generation (queue, parallel, resume)
- [ ] Automated tests (generation round-trip, export conformance)
- [ ] Documented agent workflows

**Definition of done:** an agent can drive generation → QC → export end to end
via MCP, and batch jobs run unattended.

## Open decisions

- **In-process vs server** integration with audio.cpp — decided in M2.
- **Native UI** — optional, later; Soundbox is the product-shape reference.
- **Open-source publication** — deferred until the dependency/model-weight
  review is complete. (Project license is set: Apache-2.0.)
