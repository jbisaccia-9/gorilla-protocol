#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="${HOSTING_DIR}/.env"

test -s "${ENV_FILE}" || { echo "Missing Hosting/.env." >&2; exit 1; }
set -a
# shellcheck disable=SC1090
. "${ENV_FILE}"
set +a

GAME_BINARY="${1:-${GAME_BINARY:-}}"
: "${GAME_BINARY:?Pass the packaged game path or set GAME_BINARY in Hosting/.env}"
test -x "${GAME_BINARY}" || { echo "Game binary is not executable: ${GAME_BINARY}" >&2; exit 1; }

STREAMER_URL="${STREAMER_URL:-ws://127.0.0.1:8888}"
STREAM_WIDTH="${STREAM_WIDTH:-1920}"
STREAM_HEIGHT="${STREAM_HEIGHT:-1080}"
STREAMER_WAIT_SECONDS="${STREAMER_WAIT_SECONDS:-120}"

[[ "${STREAMER_WAIT_SECONDS}" =~ ^[0-9]+$ ]] || {
  echo "STREAMER_WAIT_SECONDS must be an integer." >&2
  exit 1
}

deadline=$((SECONDS + STREAMER_WAIT_SECONDS))
until (exec 3<>/dev/tcp/127.0.0.1/8888) 2>/dev/null; do
  if (( SECONDS >= deadline )); then
    echo "Signalling did not become ready on port 8888." >&2
    exit 1
  fi
  sleep 1
done

exec "${GAME_BINARY}" \
  -PixelStreamingURL="${STREAMER_URL}" \
  -RenderOffscreen -ForceRes -ResX="${STREAM_WIDTH}" -ResY="${STREAM_HEIGHT}" \
  -AudioMixer -Unattended -StdOut -FullStdOutLogOutput
