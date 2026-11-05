# Prompting (ACE-Step 1.5 / acestep.cpp)

Praxis-Wissen aus Messungen (SFT DiT, RTX 5070, 2026-11-05).

## caption ist ein Beschreibungsblock, kein Tag

Das Repo-Beispiel `examples/full.json` ist ein einzelner Absatz in fünf
Schichten. Diese Reihenfolge liefert stabile Ergebnisse:

1. **Genre + Kern** — `aggressive thrash metal instrumental`
2. **Riffs/Instrumente** — `galloping palm-muted guitars, double-bass drums`
3. **Produktion/Ära** — `raw 90s production, punchy mix`
4. **Energie/Struktur** — `stop-start breakdowns, twin guitar harmonies`
5. **Use-Case/Vibe** — `high-octane racing intensity` (Trailing
   „perfect for …" ist explizit erlaubt und wirksam)

## Messungen (45 s, Seed fest, BPM = Onset-Autokorrelation)

| Prompt-Stil | BPM (Konfidenz) | Bemerkung |
|---|---|---|
| Kurz: „fast melodic metal" | 223 (1.18 = unbrauchbar) | kurzes Tag → unkontrolliertes Tempo |
| Ausführlich, episch (power metal) | 172 | trifft schnelleren Charakter |
| Ausführlich, Groove (metal) | 106 | trifft maskinenartigen Groove |
| Ausführlich, Symphonik + `use_cot_caption=false` | 114 (4.2) | sauberste Metadaten-Konfidenz, aber −21.8 dB RMS (leiser — CoT-Anreicherung pusht sonst Lautheit) |

## Regeln

- **Immer ausführlich prompten** (3–5 Schichten). Kurze Tags → Zufallstempo.
- `lyrics: "[Instrumental]"` = keine Vocals; `""` = LLM schreibt Lyrics.
- `--bpm` wird **nicht** vom DiT umgesetzt (Turbo und SFT gemessen identisch:
  160 angefragt → 106 real). Das LM schreibt bpm korrekt durch, der DiT
  ignoriert es — Tempo kommt aus der **Beschreibung**. Für BPM-exakte Sync-
  Musik: after-the-fact `atempo`-Resample (Plan: `--target-bpm`).
- `use_cot_caption=false`: Prompt wörtlich ans DiT — deterministischer,
  leiser. Standard `true` = LLM reichert an, lauter/„produzierter".
- Stile steuern Tempo implizit: epic/speed → ~170, groove/stomp → ~105,
  thrash/racing → ~115+.
- Seed fest für Reproduzierbarkeit; identischer Seed + Prompt = gleicher
  Track (LM-intern zwar random, praktisch stabil bei festem request-Flow).
