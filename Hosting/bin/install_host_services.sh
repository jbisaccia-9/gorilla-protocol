#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"

if (( EUID != 0 )); then
  echo "Run install_host_services.sh as root." >&2
  exit 1
fi

if ! id gorilla-stream >/dev/null 2>&1; then
  useradd --system --home-dir /var/lib/gorilla --shell /usr/sbin/nologin gorilla-stream
fi

for group in video render; do
  if getent group "${group}" >/dev/null; then
    usermod -aG "${group}" gorilla-stream
  fi
done

install -d -o gorilla-stream -g gorilla-stream -m 750 /var/lib/gorilla
if [[ -d /opt/gorilla-game ]]; then
  chown -R gorilla-stream:gorilla-stream /opt/gorilla-game
fi

install -m 644 "${HOSTING_DIR}/systemd/gorilla-streamer.service" /etc/systemd/system/gorilla-streamer.service
install -m 644 "${HOSTING_DIR}/systemd/gorilla-idle-guard.service" /etc/systemd/system/gorilla-idle-guard.service
install -m 644 "${HOSTING_DIR}/systemd/gorilla-idle-guard.timer" /etc/systemd/system/gorilla-idle-guard.timer

systemctl daemon-reload
systemctl enable gorilla-streamer.service gorilla-idle-guard.timer
systemctl restart gorilla-idle-guard.timer

echo "Streamer restart and idle shutdown services installed."
