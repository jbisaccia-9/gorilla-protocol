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
STREAM_CODEC="${STREAM_CODEC:-H264}"
STREAM_WIDTH="${STREAM_WIDTH:-1280}"
STREAM_HEIGHT="${STREAM_HEIGHT:-720}"
STREAM_FPS="${STREAM_FPS:-60}"
STREAM_MIN_BITRATE="${STREAM_MIN_BITRATE:-500000}"
STREAM_START_BITRATE="${STREAM_START_BITRATE:-6000000}"
STREAM_MAX_BITRATE="${STREAM_MAX_BITRATE:-8000000}"
STREAMER_WAIT_SECONDS="${STREAMER_WAIT_SECONDS:-120}"

[[ "${STREAM_CODEC}" == "H264" ]] || {
  echo "STREAM_CODEC must be H264 for the hardware-encoded public stream." >&2
  exit 1
}

for setting in STREAM_WIDTH STREAM_HEIGHT STREAM_FPS STREAM_MIN_BITRATE STREAM_START_BITRATE STREAM_MAX_BITRATE STREAMER_WAIT_SECONDS; do
  value="${!setting}"
  [[ "${value}" =~ ^[0-9]+$ ]] || {
    echo "${setting} must be an integer." >&2
    exit 1
  }
done

(( STREAM_MIN_BITRATE <= STREAM_START_BITRATE )) || {
  echo "STREAM_MIN_BITRATE must not exceed STREAM_START_BITRATE." >&2
  exit 1
}
(( STREAM_START_BITRATE <= STREAM_MAX_BITRATE )) || {
  echo "STREAM_START_BITRATE must not exceed STREAM_MAX_BITRATE." >&2
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
  -PixelStreamingEncoderCodec="${STREAM_CODEC}" \
  -PixelStreamingEncoderRateControl=CBR \
  -PixelStreamingWebRTCMaxFps="${STREAM_FPS}" \
  -PixelStreamingWebRTCMinBitrate="${STREAM_MIN_BITRATE}" \
  -PixelStreamingWebRTCStartBitrate="${STREAM_START_BITRATE}" \
  -PixelStreamingWebRTCMaxBitrate="${STREAM_MAX_BITRATE}" \
  -PixelStreamingWebRTCDegradationPreference=MAINTAIN_FRAMERATE \
  -RenderOffscreen -ForceRes -ResX="${STREAM_WIDTH}" -ResY="${STREAM_HEIGHT}" \
  "-ExecCmds=t.MaxFPS ${STREAM_FPS},r.MotionBlurQuality 0" \
  -AudioMixer -Unattended -StdOut -FullStdOutLogOutput
