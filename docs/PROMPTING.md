# Prompting (ACE-Step 1.5 / acestep.cpp)

Field notes from measurements (SFT DiT, RTX 5070, 2026-11-05). Short version:
**write paragraphs, not tags** — and let the description set the tempo,
because `--bpm` is decoration.

## The caption is a description block, not a tag

The repo's own showcase (`examples/full.json`) is one paragraph in five
layers. That layer order measurably works:

1. **Genre + core** — `aggressive thrash metal instrumental`
2. **Riffs / instruments** — `galloping palm-muted guitars, double-bass drums`
3. **Production / era** — `raw 90s production, punchy mix`
4. **Energy / structure** — `stop-start breakdowns, twin guitar harmonies`
5. **Use case / vibe** — `high-octane racing intensity` (a trailing
   "perfect for …" is explicitly fine and does land)

Structure markers (`[Intro]`, `[Verse]`) are irrelevant for instrumentals —
`lyrics: "[Instrumental]"` (the wrapper sets it) is enough.

## Measurements (45 s, fixed seed, BPM from onset autocorrelation)

| Prompt style | BPM (confidence) | Note |
|---|---|---|
| Short: "fast melodic metal" | 223 (1.18 = unusable) | tags → tempo roulette |
| Detailed, epic (power metal) | 172 | hits fast character |
| Detailed, groove (metal) | 106 | hits machine-like groove |
| Detailed, symphonic + `use_cot_caption=false` | 114 (4.2) | cleanest metadata confidence, but −21.8 dB RMS (quieter — CoT enrichment normally pushes loudness) |

## Rules

- **Always prompt in layers** (3–5). Short tags produce random tempo.
- `lyrics: "[Instrumental]"` = no vocals; `""` = the LM writes lyrics.
- `--bpm` is **not executed by the DiT** (measured identically on Turbo and
  SFT: 160 requested → 106 real). The LM passes `bpm` through correctly; the
  DiT ignores it. Tempo comes from the **words**. For tempo-locked sync
  music: resample afterwards with `atempo` (planned as `--target-bpm`).
- `use_cot_caption=false`: the caption goes to the DiT verbatim — more
  deterministic, quieter. Default `true` = the LM rewrites it richer and
  louder.
- Styles steer tempo implicitly: epic/speed ≈ 170, groove/stomp ≈ 105,
  thrash/racing ≈ 115+.
- Fixed seed = reproducible-enough (the LM is internally random; bit-exact
  only in DiT-only runs with identical request flow).

## Metal prompts that worked

- **Race thrash:** "aggressive thrash metal instrumental, breakneck galloping
  riffing, blast-beat drums into tight grooves, shrieking lead breaks, raw
  90s production, high octane racing intensity"
- **Boss drift:** "epic power metal instrumental, soaring harmonized guitar
  duels, thunderous double-kick, triumphant keyboard layers, heroic
  chorus-like melodies, video game boss battle energy"
- **Kart groove:** "groove metal instrumental, bouncy down-tuned riff
  pockets, head-nod half-time drums, scratchy pick attacks, oily
  machine-like swagger"
- **Rain chase:** "dark symphonic metal instrumental, orchestral strings
  over blast beats, gothic choir pads, fast tremolo guitars, cinematic
  rain-soaked chase atmosphere"
