#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
"${HOSTING_DIR}/bin/prepare_host.sh"

docker compose \
  --env-file "${HOSTING_DIR}/.env" \
  -f "${HOSTING_DIR}/docker-compose.yml" \
  up -d --build

echo "Signalling, HTTPS, STUN, and TURN services started."
echo "Start the packaged game with Hosting/bin/start_streamer_linux.sh."
