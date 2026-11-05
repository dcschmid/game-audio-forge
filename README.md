# Game Audio Forge

Lokale KI-Audio-Werkstatt für Spiele: **Soundeffekte** und **instrumentale
Musik** direkt im Terminal generieren. C++-First, keine eigenen
Audio-Frameworks — bestehende spezialisierte C++-Generatoren werden genutzt.

```
gaf music "aggressive metal racing track, fast tempo"
gaf sfx   "kart engine revving, high pitch"
gaf ui        # acestep.cpp Weboberfläche
gaf status    # GPU, Binaries, Modelle
gaf help
```

## Generatoren

| Zweck | Generator | Modell | Lizenz |
|---|---|---|---|
| Musik | [acestep.cpp](https://github.com/ServeurpersoCom/acestep.cpp) | ACE-Step 1.5, Turbo DiT + 0.6B LM (Q8_0) | MIT (Runtime + Gewichte) |
| Soundeffekte | [audiogen.cpp](https://github.com/audiohacking/audiogen.cpp) | Dasheng AudioGen (Q4) | ⚠️ Runtime ohne LICENSE-File — [Details](docs/LICENSES.md) |

Beide laufen auf **CUDA** (RTX 5070, sm_120). Keine parallele GPU-Generierung.

## Aufbau

```
game-audio-forge/
├── README.md
├── .gitignore
├── scripts/
│   ├── gaf              # CLI-Wrapper (music, sfx, ui, status, help)
│   ├── gaf-env          # Build-/Laufzeit-Umgebung (CUDA, GCC 13, sentencepiece)
│   ├── setup.sh         # One-Click-Setup (idempotent, ohne sudo)
│   ├── setup-cuda.sh    # CUDA-Toolkit 12.8 user-space installieren
│   ├── gaf-test.sh      # Quality-Gate (7 Fälle, ~70 s)
│   ├── gaf-quality.py   # BPM/RMS/HF/Corr-Analyse (stdlib + ffmpeg)
│   └── gaf-serve-install.sh  # UI als systemd-user-Dienst (Autostart)
├── config/
│   └── gaf.json         # Pfade, Modelle, Parameter, FFmpeg
├── presets/
│   ├── music/           # Musik-Presets (Prompt + Parameter)
│   └── sfx/             # SFX-Presets
└── docs/
    ├── SETUP.md         # Build-Anleitung (Fedora 44, CUDA, GCC 13)
    └── LICENSES.md      # Lizenz-Prüfung (Runtime, Gewichte, Drittkomponenten)
```

**Externe Generatoren und Modellgewichte bleiben außerhalb des Repositories**
(standardmäßig `~/dev/github/tools/`). Keine Gewichte, keine generierten
Audio-Dateien im Git.

## Audio

- Ausgabe: **WAV**, lesbare Dateinamen (`<Zeitstempel>_<Prompt-Slug>.wav`).
- **Metadaten-Sidecar** (`<name>.meta.json`): Prompt, Seed, Generator,
  Modell, Parameter, Zeitstempel.
- **Keine Überschreibung** bestehender Dateien (Kollision → `-2`, `-3`, …).
- **FFmpeg (optional):** resamplet/normalisiert (z. B. Dasheng 16 kHz → 48 kHz).
- Ausgabe-Verzeichnis: `output/{music,sfx}/` (git-ignoriert).

## Weboberfläche

`gaf ui` startet die vorhandene acestep.cpp-Weboberfläche
(`http://127.0.0.1:8085`). Keine zusätzliche gemeinsame Oberfläche.

## Setup

**One-Click:** `bash scripts/setup.sh` (idempotent, ohne sudo — micromamba,
CUDA 12.8, beide Builds, Modelle, Smoke-Test). Details: [docs/SETUP.md](docs/SETUP.md).

**UI-Dienst** (Hintergrund, Reboot-fest via systemd-user + linger):
`bash scripts/gaf-serve-install.sh` → http://127.0.0.1:8085,
Entfernen: `--remove`.

**Quality-Gate:** `scripts/gaf-test.sh` (7 deterministische Fälle mit
objektiver Messung) · Prompting-Praxis: [docs/PROMPTING.md](docs/PROMPTING.md).

Kernpunkte:

1. CUDA Toolkit 12.8 (user-space, ohne sudo) — `scripts/setup-cuda.sh`.
2. GCC 13 + sentencepiece via micromamba/conda-forge (Fedora 44 hat GCC 16,
   mit dem nvcc nicht kompiliert).
3. `source scripts/gaf-env` → beide Generatoren bauen.
4. Modelle laden (`models.sh` / `make download-models-q4`).
5. `gaf status`.

## Lizenzen

**Wichtig:** Die Runtime von `audiogen.cpp` hat **kein LICENSE-File**
(trotz Apache-2.0-Claim im README) → für kommerzielle Nutzung erst mit dem
Autor klären. acestep.cpp (Runtime + Gewichte) ist sauber MIT.
Details: [docs/LICENSES.md](docs/LICENSES.md).

## Hardware

Fedora Workstation 44, Framework Laptop 16, NVIDIA RTX 5070 Laptop (8 GB
VRAM). VRAM-Budget und Einzel-Generatoren-Strategie: [docs/SETUP.md](docs/SETUP.md).
