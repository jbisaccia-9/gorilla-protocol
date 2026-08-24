#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
docker compose \
  --env-file "${HOSTING_DIR}/.env" \
  -f "${HOSTING_DIR}/docker-compose.yml" \
  down
