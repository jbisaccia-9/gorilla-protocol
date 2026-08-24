#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="${HOSTING_DIR}/.env"
RUNTIME_DIR="${HOSTING_DIR}/runtime"

test -s "${ENV_FILE}" || {
  echo "Create Hosting/.env from Hosting/.env.example and set the real host values." >&2
  exit 1
}

set -a
# shellcheck disable=SC1090
. "${ENV_FILE}"
set +a

: "${STREAM_DOMAIN:?Set STREAM_DOMAIN in Hosting/.env}"
: "${PUBLIC_IP:?Set PUBLIC_IP in Hosting/.env}"

[[ "${STREAM_DOMAIN}" =~ ^[A-Za-z0-9.-]+$ ]] || {
  echo "STREAM_DOMAIN contains unsupported characters." >&2
  exit 1
}
[[ "${PUBLIC_IP}" =~ ^[0-9.]+$ ]] || {
  echo "PUBLIC_IP must be an IPv4 address." >&2
  exit 1
}

command -v git >/dev/null || { echo "git is required." >&2; exit 1; }
command -v openssl >/dev/null || { echo "openssl is required." >&2; exit 1; }
command -v docker >/dev/null || { echo "Docker with Compose is required." >&2; exit 1; }
docker compose version >/dev/null

PIXEL_STREAMING_INFRA_DIR="${PIXEL_STREAMING_INFRA_DIR:-${HOSTING_DIR}/.cache/PixelStreamingInfrastructure}"
if [[ "${PIXEL_STREAMING_INFRA_DIR}" != /* ]]; then
  PIXEL_STREAMING_INFRA_DIR="${HOSTING_DIR}/${PIXEL_STREAMING_INFRA_DIR#./}"
fi
export PIXEL_STREAMING_INFRA_DIR
"${HOSTING_DIR}/bin/fetch_epic_infrastructure.sh"

umask 077
mkdir -p "${RUNTIME_DIR}/secrets"
if [[ ! -s "${RUNTIME_DIR}/secrets/turn-secret" ]]; then
  openssl rand -hex 32 > "${RUNTIME_DIR}/secrets/turn-secret"
fi

turn_secret="$(tr -d '\r\n' < "${RUNTIME_DIR}/secrets/turn-secret")"
[[ "${turn_secret}" =~ ^[a-f0-9]{64}$ ]] || {
  echo "TURN secret must contain exactly 64 lowercase hexadecimal characters." >&2
  exit 1
}

sed "s/__STREAM_DOMAIN__/${STREAM_DOMAIN}/g" \
  "${HOSTING_DIR}/templates/peer-options.json.tpl" > "${RUNTIME_DIR}/peer-options.json"

sed \
  -e "s/__STREAM_DOMAIN__/${STREAM_DOMAIN}/g" \
  -e "s/__PUBLIC_IP__/${PUBLIC_IP}/g" \
  -e "s/__TURN_SECRET__/${turn_secret}/g" \
  "${HOSTING_DIR}/templates/turnserver.conf.tpl" > "${RUNTIME_DIR}/turnserver.conf"

python3 -m json.tool "${RUNTIME_DIR}/peer-options.json" >/dev/null
echo "Runtime configuration prepared without printing or committing credentials."
