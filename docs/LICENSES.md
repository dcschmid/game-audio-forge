# Lizenzen

Stand: 2026-11-04. Für spätere kommerzielle Spieleentwicklung relevant.

## Zusammenfassung

| Komponente | Lizenz | Kommerziell? | Quelle |
|---|---|---|---|
| acestep.cpp (Runtime) | MIT | ✅ Ja | [LICENSE](https://github.com/ServeurpersoCom/acestep.cpp/blob/master/LICENSE) |
| ACE-Step 1.5 Gewichte (Serveurperso/ACE-Step-1.5-GGUF) | MIT | ✅ Ja, explizit erlaubt | [HF](https://huggingface.co/Serveurperso/ACE-Step-1.5-GGUF) |
| ACE-Step 1.5 Basisgewichte (ACE-Step/Ace-Step1.5) | MIT | ✅ Ja, explizit: „use the generated music for commercial purposes" | [HF](https://huggingface.co/ACE-Step/Ace-Step1.5) |
| **audiogen.cpp (Runtime)** | **⚠️ KEIN LICENSE-File** | **❌ Nicht ohne Klärung** | [Repo](https://github.com/audiohacking/audiogen.cpp) |
| Dasheng-AudioGen Gewichte (mispeech/Dasheng-AudioGen) | Apache-2.0 | ✅ Ja | [HF](https://huggingface.co/mispeech/Dasheng-AudioGen) |
| Dasheng GGUF-Konvertierung (audiohacking/dasheng-audiogen-gguf) | Apache-2.0 (HF-Tag) | ✅ Ja | [HF](https://huggingface.co/audiohacking/dasheng-audiogen-gguf) |
| ggml (Submodul, beide Repos) | MIT | ✅ Ja | [ggml](https://github.com/ggml-org/ggml) |
| CUDA Toolkit 12.8 | NVIDIA EULA | ✅ Ja (Proprietär) | [NVIDIA](https://docs.nvidia.com/cuda/cuda-eula/index.html) |
| FFmpeg (optional) | LGPL-2.1+/GPL-2.0+ | ✅ Ja (LGPL, dynamisch) | [FFmpeg](https://ffmpeg.org/legal.html) |

## ⚠️ audiogen.cpp: Lizenz-Problem (kritisch)

Das Repository `audiohacking/audiogen.cpp` **enthält kein LICENSE-File**, obwohl das
README „Apache-2.0" angibt.

- `https://raw.githubusercontent.com/audiohacking/audiogen.cpp/main/LICENSE` → **404**
- GitHub-API: `"license": null`
- README-Banner: „⚠️ Experimental - use at your own risk!"

**Rechtliche Folge:** Ohne LICENSE-File gilt „all rights reserved" — das
Apache-2.0-Statement im README ist rechtlich nicht wirksam. Niemand (auch wir
nicht) hat das Recht, den Code zu nutzen, zu modifizieren oder zu
verwenden, **einschließlich kommerzieller Spieleentwicklung**.

**Modellgewichte sind sauber:** Die Dasheng-Gewichte (Apache-2.0) und die
GGUF-Konvertierung (Apache-2.0) sind korrekt lizenziert. Das Problem betrifft
ausschließlich den Runtime-Code.

**Aktion vor kommerzieller Nutzung:**
1. Den Autor (audiohacking-Organisation) kontaktieren und um ein
   LICENSE-File / explizite Freigabe bitten.
2. Bis dahin: audiogen.cpp **nur intern/experimentell** verwenden, keine
   generierten SFX in kommerzielle Spiele aufnehmen, die der Runtime-Code
   erzeugt.
3. Alternative: Eigenen C++-Port der Dasheng-Referenz (Apache-2.0) schreiben,
   falls der Autor nicht reagiert.

## acestep.cpp: sauber

Runtime (MIT) und alle Gewichte (MIT) sind für kommerzielle Nutzung frei.
ACE-Step 1.5 erlaubt kommerzielle Nutzung der generierten Musik explizit.

## Projekt-Lizenz

Das `game-audio-forge`-Repository selbst (Wrapper, Config, Doku) ist
unabhängig von den externen Generatoren lizenziert. Die externen
Generatoren und Modellgewichte bleiben **außerhalb** des Repositories und
unterliegen ihren eigenen Lizenzen.
