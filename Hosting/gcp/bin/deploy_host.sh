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
resolved_ips="$(dig +short A "${STREAM_DOMAIN}")"
grep -Fxq "${public_ip}" <<<"${resolved_ips}" || die "DNS for ${STREAM_DOMAIN} must resolve to ${public_ip} first."

runtime_dir="${GCP_DIR}/runtime"
env_file="${runtime_dir}/hosting.env"
umask 077
mkdir -p "${runtime_dir}"
cat >"${env_file}" <<EOF
STREAM_DOMAIN=${STREAM_DOMAIN}
PUBLIC_IP=${public_ip}
TLS_EMAIL=${TLS_EMAIL}
TURN_TTL=${TURN_TTL:-3600}
GAME_BINARY=${GAME_BINARY:-/opt/gorilla-game/GorillaProtocol.sh}
STREAMER_URL=ws://127.0.0.1:8888
STREAM_WIDTH=${STREAM_WIDTH:-1920}
STREAM_HEIGHT=${STREAM_HEIGHT:-1080}
EOF

gcloud compute scp "${env_file}" "${GCP_VM_NAME}:/tmp/gorilla-hosting.env" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap

gcloud compute ssh "${GCP_VM_NAME}" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap \
  --command="sudo git -C /opt/gorilla-protocol fetch --depth 1 origin main && sudo git -C /opt/gorilla-protocol checkout --detach FETCH_HEAD && sudo install -m 600 /tmp/gorilla-hosting.env /opt/gorilla-protocol/Hosting/.env && sudo /opt/gorilla-protocol/Hosting/bin/start_stack.sh"

echo "Hosting stack deployed. It will show a waiting screen until the packaged game connects."
