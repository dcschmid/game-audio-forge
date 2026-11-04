#!/usr/bin/env bash
# setup-cuda.sh — baut das CUDA-Toolkit 12.8 aus dem Runfile (user-space, ohne sudo).
#
# Hintergrund: Der NVIDIA-Runfile-Installer will ein GUI-Terminal (konsole)
# für die EULA. Dieses Skript extrahiert stattdessen das Payload und merged
# die Komponenten zu einem einheitlichen Toolkit-Verzeichnis.
#
# Ergebnis: $GAF_TOOLS/cuda/cuda-12.8/{bin,lib64,include,nvvm}
set -euo pipefail

GAF_TOOLS="${GAF_TOOLS:-$HOME/dev/github/tools}"
CUDA_DIR="$GAF_TOOLS/cuda"
RUNFILE="$CUDA_DIR/cuda_12.8.1_linux.run"
URL="https://developer.download.nvidia.com/compute/cuda/12.8.1/local_installers/cuda_12.8.1_570.124.06_linux.run"
DEST="$CUDA_DIR/cuda-12.8"

mkdir -p "$CUDA_DIR"

# 1) Runfile laden
if [[ ! -f "$RUNFILE" ]]; then
  echo "[setup-cuda] Lade CUDA-Runfile …"
  curl -sL -o "$RUNFILE" "$URL"
fi

# 2) Payload extrahieren (ohne Installer, ohne GUI)
EXTRACT="$CUDA_DIR/extracted"
if [[ ! -d "$EXTRACT" || -z "$(ls -A "$EXTRACT" 2>/dev/null)" ]]; then
  echo "[setup-cuda] Extrahiere Payload …"
  env -u DISPLAY -u WAYLAND_DISPLAY TERM=dumb \
    sh "$RUNFILE" --extract="$EXTRACT" < /dev/null
fi

# 3) Komponenten mergen
if [[ -x "$DEST/bin/nvcc" ]]; then
  echo "[setup-cuda] Toolkit bereits vorhanden: $DEST"
  exit 0
fi
echo "[setup-cuda] Merged Toolkit nach $DEST …"
rm -rf "$DEST"
mkdir -p "$DEST"/{bin,lib64,include,nvvm}

# bin/ direkt unter Komponenten
for d in "$EXTRACT"/*/bin; do
  [[ -d "$d" ]] && cp -rn "$d"/. "$DEST/bin/" 2>/dev/null || true
done
# targets/x86_64-linux/{bin,lib,include}
for d in "$EXTRACT"/*/targets/x86_64-linux; do
  [[ -d "$d/bin" ]]     && cp -rn "$d/bin"/.     "$DEST/bin/"     2>/dev/null || true
  [[ -d "$d/lib" ]]     && cp -rn "$d/lib"/.     "$DEST/lib64/"   2>/dev/null || true
  [[ -d "$d/include" ]] && cp -rn "$d/include"/. "$DEST/include/" 2>/dev/null || true
done
# direkte lib64/include
for d in "$EXTRACT"/*/lib64; do
  [[ -d "$d" ]] && cp -rn "$d"/. "$DEST/lib64/" 2>/dev/null || true
done
for d in "$EXTRACT"/*/include; do
  [[ -d "$d" ]] && cp -rn "$d"/. "$DEST/include/" 2>/dev/null || true
done
# nvvm (zu nvcc)
[[ -d "$EXTRACT/cuda_nvcc/nvvm" ]] && cp -rn "$EXTRACT/cuda_nvcc/nvvm"/. "$DEST/nvvm/" 2>/dev/null || true
cp "$EXTRACT/version.json" "$DEST/" 2>/dev/null || true

# 4) Verifizieren
if [[ -x "$DEST/bin/nvcc" ]]; then
  echo "[setup-cuda] OK: $($DEST/bin/nvcc --version | grep -oE 'release [0-9.]+' | head -1)"
else
  echo "[setup-cuda] FEHLER: nvcc nicht gefunden in $DEST/bin" >&2
  exit 1
fi
