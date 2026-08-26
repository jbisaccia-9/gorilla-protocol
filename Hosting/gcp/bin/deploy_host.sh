#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_stream_config
require_tools gcloud dig

public_ip="$(gcloud compute addresses describe "${GCP_ADDRESS_NAME}" \
  --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" --format='value(address)')"
if [[ "${STREAM_DOMAIN}" == "auto" ]]; then
  STREAM_DOMAIN="${public_ip//./-}.sslip.io"
fi
resolved_ips="$(dig +short A "${STREAM_DOMAIN}")"
grep -Fxq "${public_ip}" <<<"${resolved_ips}" || die "DNS for ${STREAM_DOMAIN} must resolve to ${public_ip} first."

runtime_dir="${GCP_DIR}/runtime"
env_file="${runtime_dir}/hosting.env"
umask 077
mkdir -p "${runtime_dir}"
cat >"${env_file}" <<EOF
STREAM_DOMAIN=${STREAM_DOMAIN}
PUBLIC_IP=${public_ip}
TURN_TTL=${TURN_TTL:-3600}
GAME_BINARY=${GAME_BINARY:-/opt/gorilla-game/GorillaProtocol.sh}
STREAMER_URL=ws://127.0.0.1:8888
STREAM_WIDTH=${STREAM_WIDTH:-1600}
STREAM_HEIGHT=${STREAM_HEIGHT:-900}
STREAM_FPS=${STREAM_FPS:-60}
STREAM_MIN_BITRATE=${STREAM_MIN_BITRATE:-500000}
STREAM_START_BITRATE=${STREAM_START_BITRATE:-6000000}
STREAM_MAX_BITRATE=${STREAM_MAX_BITRATE:-8000000}
IDLE_SHUTDOWN_SECONDS=${IDLE_SHUTDOWN_SECONDS:-600}
BOOT_GRACE_SECONDS=${BOOT_GRACE_SECONDS:-900}
MAX_RUNTIME_SECONDS=${MAX_RUNTIME_SECONDS:-7200}
EOF

gcloud compute scp "${env_file}" "${GCP_VM_NAME}:/tmp/gorilla-hosting.env" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap

gcloud compute ssh "${GCP_VM_NAME}" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap \
  --command="sudo /opt/gorilla-protocol/Hosting/gcp/bin/activate_release.sh /tmp/gorilla-hosting.env"

echo "Hosting stack and streamer deployed with automatic idle shutdown."
