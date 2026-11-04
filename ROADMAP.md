# Roadmap

Milestones are ordered by dependency. Each lists its deliverables and a
definition of done. M1 and M2 are complete; the next milestone is
**M3 — Audio Workspace**.

## M1 — Foundation ✅

Goal: a solid, reviewable base — repo, architecture, dev environment,
dependency strategy, license concept, documentation.

- [x] GitHub repository (private) + local git repo
- [x] Project architecture ([ARCHITECTURE.md](ARCHITECTURE.md))
- [x] Development environment notes ([docs/DEVELOPMENT.md](docs/DEVELOPMENT.md))
- [x] Dependency strategy ([docs/DEPENDENCIES.md](docs/DEPENDENCIES.md))
- [x] License concept + decision: **Apache-2.0** ([LICENSE](LICENSE),
  [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md))
- [x] Data layout contract ([docs/DATA-LAYOUT.md](docs/DATA-LAYOUT.md))
- [x] Default configuration ([config/gaf.json](config/gaf.json))
- [x] Milestones + tracking issues on GitHub
- [x] No GitHub Actions CI (by design — tests run locally)

**Definition of done:** the repository builds a clear mental model; no models
installed, no UI, no secrets committed; a contributor can read the docs and
understand the plan.

## M2 — Audio Engines ✅

Goal: prove the inference foundation on the target hardware.

- [x] Evaluate audio.cpp: run the prebuilt CUDA engine (server integration)
- [x] Test ACE-Step 1.5 (music) — generates locally on CUDA
- [x] Test MiDashengLM-Gen (SFX) — generates locally on CUDA
- [x] GPU benchmarks on RTX 5070 Laptop (8 GB): RTF + peak VRAM
  ([docs/BENCHMARKS.md](docs/BENCHMARKS.md))
- [x] Local model management: registry + on-demand download (engine endpoint)
- [x] First C++ source in `src/` + CMake build + unit tests (7 passing)
- [x] Integration decision: **server** (not in-process C ABI) — see
  [ARCHITECTURE.md](ARCHITECTURE.md)

**Definition of done:** a CLI command generates a music take and an SFX take
locally on CUDA, with measured VRAM/RTF, and models are managed from config.
**Met** — `gaf generate` produces both; benchmarks recorded.

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
