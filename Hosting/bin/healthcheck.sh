#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="${HOSTING_DIR}/.env"

test -s "${ENV_FILE}" || { echo "Missing Hosting/.env." >&2; exit 1; }
set -a
# shellcheck disable=SC1090
. "${ENV_FILE}"
set +a

: "${STREAM_DOMAIN:?Set STREAM_DOMAIN in Hosting/.env}"
curl --fail --silent --show-error --location --max-time 15 \
  "https://${STREAM_DOMAIN}/" >/dev/null

docker compose \
  --env-file "${ENV_FILE}" \
  -f "${HOSTING_DIR}/docker-compose.yml" \
  ps

echo "HTTPS player endpoint is reachable. Confirm the page reports a connected streamer."
