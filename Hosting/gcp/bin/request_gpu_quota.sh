#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud

contact_email="$(gcloud auth list --filter=status:ACTIVE --limit=1 --format='value(account)')"
test -n "${contact_email}" || die "Run gcloud auth login first."

create_or_report() {
  local preference_id="$1"
  shift
  if gcloud quotas preferences describe "${preference_id}" \
    --project="${GCP_PROJECT_ID}" --billing-project="${GCP_PROJECT_ID}" >/dev/null 2>&1; then
    echo "Quota request already exists: ${preference_id}"
    return
  fi

  gcloud quotas preferences create \
    --project="${GCP_PROJECT_ID}" \
    --billing-project="${GCP_PROJECT_ID}" \
    --service=compute.googleapis.com \
    --preferred-value=1 \
    --email="${contact_email}" \
    --justification="One on-demand L4 virtual workstation for a one-player Unreal Engine Pixel Streaming demo with automatic idle shutdown." \
    --preference-id="${preference_id}" \
    "$@"
}

create_or_report gorilla-global-gpu-1 \
  --quota-id=GPUS-ALL-REGIONS-per-project
create_or_report "gorilla-l4-vws-${GCP_REGION}-1" \
  --quota-id=NVIDIA-L4-VWS-GPUS-per-project-region \
  --dimensions="region=${GCP_REGION}"

echo "GPU quota requests submitted. No VM or billable resource was created."
