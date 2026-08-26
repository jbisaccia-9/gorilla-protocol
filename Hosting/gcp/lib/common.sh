#!/usr/bin/env bash

set -euo pipefail

GCP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_DIR="$(cd "${GCP_DIR}/../.." && pwd)"
GCP_CONFIG_FILE="${GCP_CONFIG_FILE:-${GCP_DIR}/gcp.env}"

die() {
  echo "ERROR: $*" >&2
  exit 1
}

load_config() {
  test -s "${GCP_CONFIG_FILE}" || die "Copy Hosting/gcp/gcp.env.example to Hosting/gcp/gcp.env and edit it."

  set -a
  # shellcheck disable=SC1090
  . "${GCP_CONFIG_FILE}"
  set +a

  : "${GCP_PROJECT_ID:?Set GCP_PROJECT_ID in Hosting/gcp/gcp.env}"
  : "${GCP_REGION:?Set GCP_REGION in Hosting/gcp/gcp.env}"
  : "${GCP_ZONE:?Set GCP_ZONE in Hosting/gcp/gcp.env}"

  GCP_VM_NAME="${GCP_VM_NAME:-gorilla-protocol-stream}"
  GCP_MACHINE_TYPE="${GCP_MACHINE_TYPE:-g2-standard-8}"
  GCP_ACCELERATOR_TYPE="${GCP_ACCELERATOR_TYPE:-nvidia-l4-vws}"
  GCP_NETWORK="${GCP_NETWORK:-gorilla-protocol}"
  GCP_SUBNET="${GCP_SUBNET:-gorilla-protocol-us-west1}"
  GCP_SUBNET_RANGE="${GCP_SUBNET_RANGE:-10.42.0.0/24}"
  GCP_ADDRESS_NAME="${GCP_ADDRESS_NAME:-gorilla-protocol-ip}"
  GCP_BOOT_DISK_GB="${GCP_BOOT_DISK_GB:-150}"
  GCP_MONTHLY_BUDGET_USD="${GCP_MONTHLY_BUDGET_USD:-30}"
  IDLE_SHUTDOWN_SECONDS="${IDLE_SHUTDOWN_SECONDS:-600}"
  BOOT_GRACE_SECONDS="${BOOT_GRACE_SECONDS:-900}"
  MAX_RUNTIME_SECONDS="${MAX_RUNTIME_SECONDS:-7200}"

  export GCP_VM_NAME GCP_MACHINE_TYPE GCP_ACCELERATOR_TYPE GCP_NETWORK
  export GCP_SUBNET GCP_SUBNET_RANGE GCP_ADDRESS_NAME GCP_BOOT_DISK_GB
  export GCP_MONTHLY_BUDGET_USD IDLE_SHUTDOWN_SECONDS BOOT_GRACE_SECONDS
  export MAX_RUNTIME_SECONDS

  [[ "${GCP_PROJECT_ID}" =~ ^[a-z][a-z0-9-]{4,28}[a-z0-9]$ ]] || die "GCP_PROJECT_ID is invalid."
  [[ "${GCP_REGION}" =~ ^[a-z]+-[a-z]+[0-9]+$ ]] || die "GCP_REGION is invalid."
  [[ "${GCP_ZONE}" =~ ^${GCP_REGION}-[a-z]$ ]] || die "GCP_ZONE must belong to GCP_REGION."
  [[ "${GCP_VM_NAME}" =~ ^[a-z]([-a-z0-9]*[a-z0-9])?$ ]] || die "GCP_VM_NAME is invalid."
  [[ "${GCP_MACHINE_TYPE}" =~ ^g2-(standard|custom)-[a-z0-9-]+$ ]] || die "Use a G2 machine type."
  [[ "${GCP_ACCELERATOR_TYPE}" =~ ^nvidia-l4(-vws)?$ ]] || die "Use nvidia-l4 or nvidia-l4-vws."
  [[ "${GCP_BOOT_DISK_GB}" =~ ^[0-9]+$ ]] || die "GCP_BOOT_DISK_GB must be an integer."
  (( GCP_BOOT_DISK_GB >= 40 )) || die "G2 boot disks must be at least 40 GB."
  [[ "${GCP_MONTHLY_BUDGET_USD}" =~ ^[0-9]+([.][0-9]+)?$ ]] || die "GCP_MONTHLY_BUDGET_USD must be a positive number."
  [[ "${IDLE_SHUTDOWN_SECONDS}" =~ ^[0-9]+$ ]] || die "IDLE_SHUTDOWN_SECONDS must be an integer."
  [[ "${BOOT_GRACE_SECONDS}" =~ ^[0-9]+$ ]] || die "BOOT_GRACE_SECONDS must be an integer."
  [[ "${MAX_RUNTIME_SECONDS}" =~ ^[0-9]+$ ]] || die "MAX_RUNTIME_SECONDS must be an integer."
  (( IDLE_SHUTDOWN_SECONDS >= 300 )) || die "IDLE_SHUTDOWN_SECONDS must be at least 300."
  (( BOOT_GRACE_SECONDS >= IDLE_SHUTDOWN_SECONDS )) || die "BOOT_GRACE_SECONDS must be at least IDLE_SHUTDOWN_SECONDS."
  (( MAX_RUNTIME_SECONDS >= BOOT_GRACE_SECONDS )) || die "MAX_RUNTIME_SECONDS must be at least BOOT_GRACE_SECONDS."
}

require_tools() {
  local tool
  for tool in "$@"; do
    command -v "${tool}" >/dev/null || die "${tool} is required."
  done
}

require_apply() {
  [[ "${1:-}" == "--apply" ]] || die "This changes GCP resources. Review it, then rerun with --apply."
}

require_stream_config() {
  : "${STREAM_DOMAIN:?Set STREAM_DOMAIN in Hosting/gcp/gcp.env}"
  TURN_TTL="${TURN_TTL:-3600}"
  STREAM_WIDTH="${STREAM_WIDTH:-1600}"
  STREAM_HEIGHT="${STREAM_HEIGHT:-900}"
  STREAM_FPS="${STREAM_FPS:-60}"
  STREAM_MIN_BITRATE="${STREAM_MIN_BITRATE:-500000}"
  STREAM_START_BITRATE="${STREAM_START_BITRATE:-6000000}"
  STREAM_MAX_BITRATE="${STREAM_MAX_BITRATE:-8000000}"

  [[ "${STREAM_DOMAIN}" == "auto" || "${STREAM_DOMAIN}" =~ ^[A-Za-z0-9.-]+$ ]] || die "STREAM_DOMAIN is invalid."
  [[ "${GAME_BINARY:-/opt/gorilla-game/GorillaProtocol.sh}" =~ ^/[A-Za-z0-9._/-]+$ ]] || die "GAME_BINARY must be a safe absolute path without spaces."
  [[ "${TURN_TTL}" =~ ^[0-9]+$ ]] || die "TURN_TTL must be an integer."
  [[ "${STREAM_WIDTH}" =~ ^[0-9]+$ ]] || die "STREAM_WIDTH must be an integer."
  [[ "${STREAM_HEIGHT}" =~ ^[0-9]+$ ]] || die "STREAM_HEIGHT must be an integer."
  [[ "${STREAM_FPS}" =~ ^[0-9]+$ ]] || die "STREAM_FPS must be an integer."
  [[ "${STREAM_MIN_BITRATE}" =~ ^[0-9]+$ ]] || die "STREAM_MIN_BITRATE must be an integer."
  [[ "${STREAM_START_BITRATE}" =~ ^[0-9]+$ ]] || die "STREAM_START_BITRATE must be an integer."
  [[ "${STREAM_MAX_BITRATE}" =~ ^[0-9]+$ ]] || die "STREAM_MAX_BITRATE must be an integer."
  (( STREAM_FPS >= 30 && STREAM_FPS <= 120 )) || die "STREAM_FPS must be between 30 and 120."
  (( STREAM_MIN_BITRATE <= STREAM_START_BITRATE )) || die "STREAM_MIN_BITRATE must not exceed STREAM_START_BITRATE."
  (( STREAM_START_BITRATE <= STREAM_MAX_BITRATE )) || die "STREAM_START_BITRATE must not exceed STREAM_MAX_BITRATE."
}

quota_record() {
  local scope="$1"
  local metric="$2"
  local output

  if [[ "${scope}" == "global" ]]; then
    output="$(gcloud compute project-info describe \
      --project="${GCP_PROJECT_ID}" \
      --flatten='quotas[]' \
      --format='value(quotas.metric,quotas.limit,quotas.usage)')"
  else
    output="$(gcloud compute regions describe "${GCP_REGION}" \
      --project="${GCP_PROJECT_ID}" \
      --flatten='quotas[]' \
      --format='value(quotas.metric,quotas.limit,quotas.usage)')"
  fi

  awk -v wanted="${metric}" '$1 == wanted { print $2, $3; found=1 } END { if (!found) print "0 0" }' <<<"${output}"
}

quota_limit() {
  local quota_id="$1"
  local region="${2:-}"
  local quota_json

  quota_json="$(gcloud quotas info describe "${quota_id}" \
    --service=compute.googleapis.com \
    --project="${GCP_PROJECT_ID}" \
    --format=json)"

  python3 -c '
import json
import sys

data = json.load(sys.stdin)
region = sys.argv[1]
for item in data.get("dimensionsInfos", []):
    dimensions = item.get("dimensions", {})
    if not region or dimensions.get("region") == region:
        print(item.get("details", {}).get("value", "0"))
        raise SystemExit(0)
print("0")
' "${region}" <<<"${quota_json}"
}

resource_exists() {
  "$@" >/dev/null 2>&1
}
