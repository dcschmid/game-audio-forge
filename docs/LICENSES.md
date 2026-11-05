# Licenses

As of 2026-11-04. Everything here matters because the end goal is
**commercial game development** — so we checked runtime code, model weights
and third-party pieces alike.

## Summary

| Component | License | Commercial use? | Source |
|---|---|---|---|
| acestep.cpp (runtime) | MIT | ✅ Yes | [LICENSE](https://github.com/ServeurpersoCom/acestep.cpp/blob/master/LICENSE) |
| ACE-Step 1.5 GGUF weights (Serveurperso/ACE-Step-1.5-GGUF) | MIT | ✅ Yes | [HF](https://huggingface.co/Serveurperso/ACE-Step-1.5-GGUF) |
| ACE-Step 1.5 base weights (ACE-Step/Ace-Step1.5) | MIT | ✅ Yes — "use the generated music for commercial purposes" | [HF](https://huggingface.co/ACE-Step/Ace-Step1.5) |
| **audiogen.cpp (runtime)** | **⚠️ no LICENSE file** | **❌ not until clarified** | [repo](https://github.com/audiohacking/audiogen.cpp) |
| Dasheng-AudioGen weights (mispeech/Dasheng-AudioGen) | Apache-2.0 | ✅ Yes | [HF](https://huggingface.co/mispeech/Dasheng-AudioGen) |
| Dasheng GGUF conversions (audiohacking/dasheng-audiogen-gguf) | Apache-2.0 (HF tag) | ✅ Yes | [HF](https://huggingface.co/audiohacking/dasheng-audiogen-gguf) |
| ggml (submodule in both repos) | MIT | ✅ Yes | [ggml](https://github.com/ggml-org/ggml) |
| CUDA Toolkit 12.8 | NVIDIA EULA | ✅ Yes (proprietary) | [EULA](https://docs.nvidia.com/cuda/cuda-eula/index.html) |
| FFmpeg (optional) | LGPL-2.1+/GPL-2.0+ | ✅ Yes (dynamic linking) | [legal](https://ffmpeg.org/legal.html) |

## ⚠️ audiogen.cpp: the one blocker

`audiohacking/audiogen.cpp` **contains no LICENSE file**, even though its
README says "Apache-2.0" and carries an "Experimental — use at your own risk"
banner.

- `https://raw.githubusercontent.com/audiohacking/audiogen.cpp/main/LICENSE` → **404**
- GitHub API: `"license": null`

**Legal consequence:** without a license file the default is "all rights
reserved". The Apache-2.0 claim in a README is not an effective grant —
nobody (including us) may use, modify or ship the code, **including in
commercial games**, until the author says otherwise.

The **weights are clean**: both the original Dasheng weights and the GGUF
conversions are properly Apache-2.0. The issue is only the runtime code.

**Before commercial use:**
1. Ask the audiohacking org to add a LICENSE file / explicit grant.
2. Until then: internal/experimental use only; don't ship runtime-produced
   SFX in commercial builds.
3. If the author stays silent: port the Apache-2.0 Python reference to C++
   ourselves (the conversion scripts in `convert/` are Apache-2.0).

## acestep.cpp: clean

Runtime MIT, all weights MIT. ACE-Step 1.5 explicitly allows commercial use
of generated music. Green light.

## This repository

`game-audio-forge` itself (wrapper, config, docs) is licensed independently.
External generators and weights stay **outside** the repo under their own
terms.
