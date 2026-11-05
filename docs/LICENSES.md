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
| **sa3.cpp (SFX runtime, default)** | MIT | ✅ Yes | [LICENSE](https://github.com/betweentwomidnights/sa3.cpp/blob/main/LICENSE) |
| Stable Audio 3 small-sfx weights (stabilityai + thepatch GGUFs) | Stability AI Community License | ✅ Yes — **while annual revenue < USD 1M**; above: enterprise license, registration + "Powered by Stability AI" notice | [LICENSE.md](https://huggingface.co/stabilityai/stable-audio-open-1.0/blob/main/LICENSE.md) |
| audiogen.cpp (runtime, legacy backend) | **⚠️ no LICENSE file** | **❌ not until clarified** | [repo](https://github.com/audiohacking/audiogen.cpp) |
| Dasheng-AudioGen weights (mispeech/Dasheng-AudioGen) | Apache-2.0 | ✅ Yes | [HF](https://huggingface.co/mispeech/Dasheng-AudioGen) |
| Dasheng GGUF conversions (audiohacking/dasheng-audiogen-gguf) | Apache-2.0 (HF tag) | ✅ Yes | [HF](https://huggingface.co/audiohacking/dasheng-audiogen-gguf) |
| ggml (submodule in the repos) | MIT | ✅ Yes | [ggml](https://github.com/ggml-org/ggml) |
| CUDA Toolkit 12.8 | NVIDIA EULA | ✅ Yes (proprietary) | [EULA](https://docs.nvidia.com/cuda/cuda-eula/index.html) |
| FFmpeg (optional) | LGPL-2.1+/GPL-2.0+ | ✅ Yes (dynamic linking) | [legal](https://ffmpeg.org/legal.html) |

## Default SFX chain: sa3.cpp + Stable Audio 3 small-sfx

`betweentwomidnights/sa3.cpp` carries a real MIT LICENSE file (verified 2026-11-05).
The SFX weights are the catch-free-under-a-million tier: the Stability AI
Community License grants free commercial use — including shipped games — as
long as you and your affiliates stay under **USD 1M annual revenue**; above
that you need an enterprise license, a registration and a "Powered by
Stability AI" notice. For this workshop (solo/small-team game dev) that is a
non-issue, but it is **not** MIT/Apache-class freedom: there is no SFX model
today that combines permissive weights **and** a C++ runtime **and** 44.1 kHz
stereo — MMAudio, AudioLDM2, MusicGen, AudioX and TangoFlux all ship
CC-BY-NC-class weights.

## ⚠️ audiogen.cpp: legacy backend, runtime still blocked

`audiohacking/audiogen.cpp` **still contains no LICENSE file** (re-verified
2026-11-05: LICENSE → 404 on `main` and `master`, GitHub API `license: null`),
even though its README says "Apache-2.0".

**Legal consequence:** without a license file the default is "all rights
reserved". The Apache-2.0 claim in a README is not an effective grant —
do not ship runtime-produced SFX in commercial builds until the author grants
it. The **weights are clean** (Apache-2.0); only the runtime is the issue.
`gaf sfx --backend dasheng` uses this chain; it stays available for internal
experiments and A/B comparisons.

**Before commercial use:** ask the author for a LICENSE file; or port the
Apache-2.0 reference yourself. Until then: internal use only.

## acestep.cpp: clean

Runtime MIT, all weights MIT. ACE-Step 1.5 explicitly allows commercial use
of generated music. Green light.

## This repository

`game-audio-forge` itself (wrapper, config, docs) is licensed independently.
External generators and weights stay **outside** the repo under their own
terms.
