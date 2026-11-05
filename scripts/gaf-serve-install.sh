#!/usr/bin/env bash
# gaf-serve-install.sh — acestep.cpp web UI as a user service (systemd).
#
# runs in the background, restarts after reboot (linger enabled),
# no sudo. usage:
#   bash scripts/gaf-serve-install.sh            # installieren + starten
#   bash scripts/gaf-serve-install.sh --remove   # deinstallieren
#
# Dienst:  gaf-audio-ui.service  (Port aus config/gaf.json → ui.port)
# Logs:    journalctl --user -u gaf-audio-ui -f
set -uo pipefail
GAF_HOME="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UNIT_DIR="$HOME/.config/systemd/user"
UNIT="$UNIT_DIR/gaf-audio-ui.service"
CONF="$GAF_HOME/config/gaf.json"
TOOLS="${GAF_TOOLS:-$HOME/dev/github/tools}"
ENVF="$GAF_HOME/scripts/gaf-env"

remove() {
  systemctl --user disable --now gaf-audio-ui.service 2>/dev/null || true
  rm -f "$UNIT"; systemctl --user daemon-reload
  echo "[gaf-serve] deinstalliert"
}
[[ "${1:-}" == "--remove" ]] && { remove; exit 0; }

HOST="$(jq -r '.ui.host // "127.0.0.1"' "$CONF")"
PORT="$(jq -r '.ui.port // 8085' "$CONF")"
[[ -n "$HOST" && "$HOST" != null ]] || HOST="127.0.0.1"
[[ -n "$PORT" && "$PORT" != null ]] || PORT=8085

mkdir -p "$UNIT_DIR"
cat > "$UNIT" <<EOF
[Unit]
Description=Game Audio Forge — acestep.cpp WebUI (Port $PORT)
After=network.target

[Service]
Type=simple
WorkingDirectory=$GAF_HOME
ExecStart=/usr/bin/env bash -lc 'source $ENVF && exec $TOOLS/acestep.cpp/build/ace-server --host $HOST --port $PORT --models $TOOLS/acestep.cpp/models --adapters $TOOLS/acestep.cpp/adapters --max-batch 1'
Restart=on-failure
RestartSec=3
# the UI server holds modules on demand — VRAM-friendly, self-evicting.
Environment=PYTHONUNBUFFERED=1

[Install]
WantedBy=default.target
EOF

systemctl --user daemon-reload
systemctl --user enable --now gaf-audio-ui.service

# user instances must survive logout/reboot:
if loginctl enable-linger "$USER" 2>/dev/null; then
  echo "[gaf-serve] linger enabled (service survives logout/reboot)"
else
  echo "[gaf-serve] WARNING: cannot enable linger — service starts only after login"
fi

sleep 2
if systemctl --user is-active --quiet gaf-audio-ui.service; then
  echo "[gaf-serve] OK: http://$HOST:$PORT  (Logs: journalctl --user -u gaf-audio-ui -f)"
else
  echo "[gaf-serve] service did not start — status:"
  systemctl --user status gaf-audio-ui.service --no-pager | tail -8
fi
