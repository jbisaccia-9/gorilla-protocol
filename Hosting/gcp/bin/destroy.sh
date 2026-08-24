#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_tools gcloud
[[ "${1:-}" == "--apply" && "${2:-}" == "${GCP_PROJECT_ID}" ]] || {
  die "Destructive cleanup. Run: destroy.sh --apply ${GCP_PROJECT_ID}"
}

if resource_exists gcloud compute instances describe "${GCP_VM_NAME}" --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"; then
  gcloud compute instances delete "${GCP_VM_NAME}" --quiet \
    --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
fi

if resource_exists gcloud compute addresses describe "${GCP_ADDRESS_NAME}" --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"; then
  gcloud compute addresses delete "${GCP_ADDRESS_NAME}" --quiet \
    --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"
fi

for rule in gorilla-stream-web gorilla-stream-turn gorilla-stream-media gorilla-stream-iap-ssh; do
  if resource_exists gcloud compute firewall-rules describe "${rule}" --project="${GCP_PROJECT_ID}"; then
    gcloud compute firewall-rules delete "${rule}" --quiet --project="${GCP_PROJECT_ID}"
  fi
done

if resource_exists gcloud compute networks subnets describe "${GCP_SUBNET}" --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"; then
  gcloud compute networks subnets delete "${GCP_SUBNET}" --quiet \
    --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"
fi

if resource_exists gcloud compute networks describe "${GCP_NETWORK}" --project="${GCP_PROJECT_ID}"; then
  gcloud compute networks delete "${GCP_NETWORK}" --quiet --project="${GCP_PROJECT_ID}"
fi

echo "Gorilla Protocol GCP runtime resources deleted."
