# GPU Benchmarks

Measured on the primary development hardware. These numbers are the basis for
the 8 GB VRAM budget decisions in [ARCHITECTURE.md](../ARCHITECTURE.md).

## Hardware

| | |
|---|---|
| OS | Fedora Workstation 44 |
| CPU | AMD Ryzen AI 9 HX 370 (24 threads) |
| GPU | NVIDIA GeForce RTX 5070 Laptop GPU |
| VRAM | 8151 MiB (7799 MiB usable) |
| Compute capability | 12.0 (Blackwell) |
| Driver | 615.78.08 |
| Engine | audio.cpp v0.9.3 (prebuilt CUDA 12.8 binary) |
| Backend | CUDA |

## Models tested

| Model | Package | Precision | Weights | Task |
|---|---|---|---|---|
| ACE-Step 1.5 Turbo | `ace_step_turbo_q8_0` | Q8_0 | 6.19 GB | instrumental music |
| MiDashengLM-Gen | `midashenglm_gen_q8_0` | Q8_0 | 3.38 GB | sound effects / ambience |

Both are the Q8_0 (low-VRAM) variants. BF16 variants are ~1.6× larger and do
**not** fit the 8 GB budget for ACE-Step.

## Results

| Model | Output | Inference time | RTF | Peak VRAM | Fits 8 GB |
|---|---|---|---|---|---|
| MiDashengLM-Gen (SFX) | 20 s, 16 kHz mono | 4.85 s | 0.24 | **3805 MiB** | ✅ |
| ACE-Step 1.5 Turbo (music) | 205 s, 48 kHz stereo | 26.0 s | 0.13 | **6947 MiB** | ✅ (tight) |

RTF = inference time / audio duration. Lower is better; < 1.0 means faster
than real-time. (Inference time excludes model load and CLI overhead.)

### Notes

- **First run includes model load.** The first SFX request took ~73 s (model
  load + inference); subsequent runs with the model resident are ~5 s.
- **ACE-Step needs `mem_saver: true`** to fit. Without it, the DiT weight
  buffer allocation fails (planner 2.27 GB + VAE 0.44 GB + DiT exceed 8 GB).
  `mem_saver` releases staged state between phases; later requests may rebuild
  it (slightly slower).
- **`max_loaded_models: 1`** keeps only one model resident, so switching
  models unloads the previous one and frees VRAM.
- **ACE-Step duration:** the planner chooses the track length; a
  `duration_sec` request is a budget, not a hard cap (a 24 s request produced a
  205 s track). Loop trimming to an exact length is an M4 (Processing) task.
- **MiDashengLM-Gen duration:** likewise derives from the checkpoint sequence
  length; a 3 s request produced 20 s.

## Reproduce

```bash
# Build the CLI
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -GNinja && cmake --build build -j

# Start the engine server (see docs/DEVELOPMENT.md)
# then:
scripts/gaf-benchmark.sh midashenglm_gen "heavy wooden door slamming" 3 42
scripts/gaf-benchmark.sh ace_step "dark ambient cave drone, seamless loop" 24 7
```

The script samples `nvidia-smi` once per second and reports peak VRAM plus the
engine-reported RTF.

## Implications for the 8 GB budget

- **Both default models fit** at Q8_0 with the settings above.
- **MiniMax Music 3 (~10 GB)** does **not** fit — off by default
  (`config/gaf.json`).
- **Stable Audio 3.0** (SFX + Music) is the next model to benchmark; expected
  to fit at Q8_0 (smaller than ACE-Step).
- Running two large models simultaneously is **not** possible on 8 GB; the
  single-resident-model policy is required.
