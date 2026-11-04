# Data Layout

Game Audio Forge keeps **all large data outside the source repository**. The
repository holds only configuration, presets, example metadata, documentation
and source code. Everything heavy — model weights, generated audio, masters,
temp inference data — lives in an XDG-compatible directory tree.

## What stays in the repo

- Source code (`src/`), build files, scripts, tests.
- Shipped default configuration (`config/gaf.json`).
- Shipped presets (`presets/`) and example profiles/metadata (`examples/`).
- Documentation (`docs/`, `*.md`).

## What never goes in the repo

- Model weights: `*.gguf`, `*.ggml`, `*.safetensors`, `*.ckpt`, `*.bin`,
  `*.onnx`, …
- Audio: `*.wav`, `*.flac`, `*.mp3`, `*.ogg`, `*.m4a`, `*.aiff`, `*.opus`.
- Generated sound libraries, music files, WAV/FLAC masters.
- Temp inference data, caches, logs.
- Secrets, tokens, machine-specific config.

The `.gitignore` enforces this. Do not weaken it.

## XDG directory contract

Paths resolve in this order (first match wins):

1. An explicit absolute path in `config/gaf.json` (top-level keys).
2. The relevant environment variable.
3. The XDG base-directory default.

| Purpose | Env var | Default |
|---|---|---|
| Data root | `GAF_DATA_DIR` → `XDG_DATA_HOME` | `~/.local/share/game-audio-forge` |
| Config | `GAF_CONFIG_DIR` → `XDG_CONFIG_HOME` | `~/.config/game-audio-forge` |
| Cache | `GAF_CACHE_DIR` → `XDG_CACHE_HOME` | `~/.cache/game-audio-forge` |

If `XDG_DATA_HOME` is unset, the default is `$HOME/.local/share`; likewise
`$HOME/.config` and `$HOME/.cache` for the others.

## Directory tree

```
$XDG_DATA_HOME/game-audio-forge/
├── models/                  # model weights (downloaded on demand)
│   └── <family>/            #   e.g. ace_step/, midashenglm_gen/
├── libraries/               # generated + imported audio, per profile
│   └── <profile>/
│       ├── sfx/
│       ├── music/
│       └── library.json     #   take metadata for this profile
├── projects/                # project profile definitions (*.json)
├── presets/                 # user presets (shipped ones live in repo /presets)
└── cache/                   # temp inference data (scratch)

$XDG_CONFIG_HOME/game-audio-forge/
├── gaf.json                 # user config (copy of repo default + overrides)
├── gaf.local.json           # machine-specific overrides (gitignored)
└── state.json               # active profile (app state)

$XDG_CACHE_HOME/game-audio-forge/
└── downloads/               # model download cache (partial files, etc.)
```

## Library record

Each take in a profile library is an entry in `library.json` (a JSON array)
plus an audio file on disk:

| Field | Description |
|---|---|
| `id` | stable id (ULID) |
| `profile` | owning profile |
| `kind` | `sfx` \| `music` \| `loop` \| `import` |
| `path` | audio file path (relative to the profile dir) |
| `prompt` | generation prompt |
| `model` | model family + package |
| `seed` | generation seed (if applicable) |
| `duration_s` | duration |
| `sample_rate` | sample rate |
| `bit_depth` | bit depth |
| `license` | model license (recorded at generation) |
| `tags` | free-form tags |
| `created_at` | timestamp |
| `peak` | peak amplitude (0–1) |
| `rms` | root-mean-square amplitude |
| `loudness_lufs` | approximate loudness (RMS-based; true LUFS is M4) |
| `silence_head_s` | leading silence duration |
| `silence_tail_s` | trailing silence duration |
| `qc_ok` | false if QC flagged the take (silent or over-loud) |

The QC fields are filled automatically by the `qc` module after every
generation and import.

## Rationale

- **XDG** keeps the layout predictable on Linux and overridable for tests and
  packaging.
- **Per-profile libraries** give independent project scopes while sharing the
  model cache (models are downloaded once, used by all profiles).
- **JSON library** keeps the store dependency-free and human-readable for M3.
  The API is small; a later migration to SQLite (for a cross-profile "central"
  index) is cheap and is an M4+ option.
- **Gitignored by default** means a stray `git add -A` cannot leak weights or
  audio.
