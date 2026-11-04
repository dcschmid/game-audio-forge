# Contributing

Thanks for helping build Game Audio Forge. This is a local-first, C++-first
project; the rules below keep it that way.

## Ground rules

- **C++-first, no mandatory Python.** Inference goes through audio.cpp. Python
  is allowed only for optional reference tooling and must never become a
  runtime or build dependency.
- **CUDA-first, backend-open.** Never hard-code CUDA. Backend selection stays
  runtime-configurable so Vulkan, ROCm and CPU keep working.
- **Thin over audio.cpp.** Do not re-implement model inference. audio.cpp owns
  inference; our code owns the product (library, profiles, presets, QC,
  export, MCP).
- **Assets never in the repo.** Model weights, GGUF files, generated audio,
  WAV/FLAC masters and temp data live in the XDG data directory, not in git.
  The `.gitignore` enforces this — do not weaken it.
- **No secrets.** Never commit tokens, credentials, machine-specific paths or
  local config. CI runs a secret scan.
- **Local-first, private by default.** No telemetry, no accounts, no
  phone-home. Network access is limited to explicit model downloads.

## Development setup

See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md). In short:

```bash
# Fedora 44, GCC 13+, CMake, Ninja, CUDA Toolkit, FFmpeg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DAUDIOCPP_BACKEND=cuda
cmake --build build -j
ctest --test-dir build
```

## Code style

- Follow [.editorconfig](.editorconfig): 4-space indent for C++, 2 for
  config/docs, LF line endings, final newline, no trailing whitespace.
- C++20. Prefer clarity over cleverness; this is a thin layer, not a
  framework.
- Keep the product layer small. If a change feels like it belongs in audio.cpp,
  open an issue upstream instead of forking.

## Testing

- Unit tests (no GPU) must pass in CI.
- Integration tests that need a GPU or a downloaded model must be skippable
  and clearly marked.
- Add a regression test for any bug fix.

## Pull requests

- Small, focused PRs. One concern per PR.
- Update docs when behavior changes (README, ARCHITECTURE, ROADMAP, CHANGELOG).
- No model weights or large audio in a PR.
- Reference the tracking issue in the PR description.

## License

The project is licensed under **Apache License 2.0** (see
[LICENSE](LICENSE)). Contributions are licensed under the same terms. Do not
add code under a different license without discussion.
