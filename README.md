# Game Audio Forge

Your local audio workshop for game development. Music and sound effects,
generated on your own GPU, driven entirely from the terminal — no cloud, no
subscriptions, no audio framework of your own to maintain.

```bash
gaf music "aggressive metal racing track, galloping guitars, double-kick drums"
gaf sfx "go-kart engine revving, small four-stroke piston, high pitch"
gaf ui        # the acestep.cpp web interface on http://127.0.0.1:8085
gaf status    # GPU, binaries, models at a glance
```

The idea is small on purpose: two specialised C++ generators do the heavy
lifting, a thin `gaf` wrapper makes them pleasant to use.

## What runs under the hood

| | Generator | Model | License |
|---|---|---|---|
| **Music** | [acestep.cpp](https://github.com/ServeurpersoCom/acestep.cpp) | ACE-Step 1.5, SFT DiT + 0.6B LM (Q8_0) | MIT — runtime *and* weights, commercial use explicitly fine |
| **Sound effects** | [sa3.cpp](https://github.com/betweentwomidnights/sa3.cpp) | Stable Audio 3 Small SFX (Q8_0), 44.1 kHz stereo | runtime MIT; weights Stability Community License (free commercial < $1M/yr) |

Both build against **CUDA** (RTX 5070 Laptop, 8 GB) and never run on the GPU
at the same time — `gaf` queues them for you. `gaf sfx --backend dasheng`
switches to the legacy 16 kHz Dasheng chain (Apache-2.0 weights, runtime
unlicensed — see docs).

A 30-second music track takes ~**9 s** at ~4.5 GB VRAM, a 5-second SFX ~**2 s**.
Full measurements in [docs/PROMPTING.md](docs/PROMPTING.md).

## Repository layout

```
game-audio-forge/
├── README.md
├── .gitignore
├── scripts/
│   ├── gaf                   # the CLI (music, sfx, ui, status, help)
│   ├── gaf-env               # sources the build/runtime env (CUDA, GCC 13, …)
│   ├── setup.sh              # one-click install (idempotent, no sudo)
│   ├── setup-cuda.sh         # CUDA 12.8 toolkit into user space
│   ├── gaf-test.sh           # quality gate: 7 deterministic cases, ~70 s
│   ├── gaf-quality.py        # BPM / RMS / HF / correlation analyser
│   └── gaf-serve-install.sh  # UI as a systemd user service (survives reboot)
├── config/
│   └── gaf.json              # paths, models, parameters, FFmpeg
├── presets/
│   ├── music/                # music presets (prompt + parameters)
│   └── sfx/                  # sound-effect presets
└── docs/
    ├── SETUP.md              # build guide (Fedora 44, CUDA, sm_120a)
    ├── PROMPTING.md          # what actually moves the model
    └── LICENSES.md           # runtime, weights, third-party audit
```

Generators and model weights live **outside** the repository
(`~/dev/github/tools/` by default). No weights, no audio in Git — ever.

## Getting started

```bash
bash scripts/setup.sh                        # one command, no sudo, re-runnable
source scripts/gaf-env                       # then, in every new shell:
scripts/gaf music "your track" --duration 45 --seed 11
```

Everything the setup needs beyond `git`, `cmake` and an NVIDIA driver is
installed into your home directory: CUDA 12.8 (extracted from the official
runfile), GCC 13 (Fedora 44 ships GCC 16, which nvcc cannot digest) and
sentencepiece.

### The web interface (optional, but nice)

You don't have to live in the terminal. One command installs the existing
acestep.cpp UI as a **systemd user service** that comes back after every
reboot:

```bash
bash scripts/gaf-serve-install.sh            # → http://127.0.0.1:8085
bash scripts/gaf-serve-install.sh --remove   # uninstall
```

## Audio output

- **WAV**, readable names: `<timestamp>_<prompt-slug>.wav`
- a **`.meta.json` sidecar** with prompt, seed and every parameter — so a
  track is reproducible days later
- existing files are **never overwritten** (collisions become `-2`, `-3`, …)
- optional **FFmpeg** normalisation (`ffmpeg.target_rate`, e.g. → 48 kHz)
- output directory: `output/{music,sfx}/` (git-ignored)

## Quality gate

How good is "good"? `scripts/gaf-test.sh` answers with measurements: seven
deterministic cases (three music styles, cover, repaint, two SFX), each
checked for duration, task type, loudness and high-frequency content. BPM is
reported via onset autocorrelation — measured, not guessed.

```bash
scripts/gaf-test.sh        # → 7/7 BESTANDEN + JSON report in output/tests/
python3 scripts/gaf-quality.py output/music/some-track.wav
```

## Licensing — read before shipping

Music is clean: acestep.cpp and all ACE-Step weights are MIT, commercial use
explicitly allowed. SFX default (sa3.cpp) is MIT runtime; the Stable Audio 3
weights are **free for commercial use under $1M annual revenue** (attribution
plus registration above that). The legacy Dasheng chain has Apache-2.0 weights
but its **runtime repo has no LICENSE file** — keep it out of commercial
builds unless clarified. Details: [docs/LICENSES.md](docs/LICENSES.md).

## Target hardware

Fedora Workstation 44, Framework Laptop 16, NVIDIA RTX 5070 Laptop (8 GB
VRAM). VRAM budget and the reasoning behind sequential generation:
[docs/SETUP.md](docs/SETUP.md).
