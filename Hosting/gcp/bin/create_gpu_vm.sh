#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud
"${SCRIPT_DIR}/preflight.sh"

resource_exists gcloud compute networks describe "${GCP_NETWORK}" --project="${GCP_PROJECT_ID}" || die "Run create_foundation.sh --apply first."
resource_exists gcloud compute addresses describe "${GCP_ADDRESS_NAME}" --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" || die "Run create_foundation.sh --apply first."

if resource_exists gcloud compute instances describe "${GCP_VM_NAME}" --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"; then
  die "VM ${GCP_VM_NAME} already exists."
fi

public_ip="$(gcloud compute addresses describe "${GCP_ADDRESS_NAME}" \
  --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" --format='value(address)')"

gcloud compute instances create "${GCP_VM_NAME}" \
  --project="${GCP_PROJECT_ID}" \
  --zone="${GCP_ZONE}" \
  --machine-type="${GCP_MACHINE_TYPE}" \
  --accelerator="type=${GCP_ACCELERATOR_TYPE},count=1" \
  --maintenance-policy=TERMINATE \
  --restart-on-failure \
  --provisioning-model=STANDARD \
  --boot-disk-type=pd-balanced \
  --boot-disk-size="${GCP_BOOT_DISK_GB}GB" \
  --image-family=ubuntu-2204-lts \
  --image-project=ubuntu-os-cloud \
  --network="${GCP_NETWORK}" \
  --subnet="${GCP_SUBNET}" \
  --address="${public_ip}" \
  --network-tier=PREMIUM \
  --tags=gorilla-stream-edge,gorilla-iap-ssh \
  --labels=application=gorilla-protocol,role=pixel-streamer \
  --metadata=enable-oslogin=TRUE \
  --metadata-from-file="startup-script=${SCRIPT_DIR}/../startup.sh" \
  --no-service-account \
  --no-scopes \
  --no-shielded-secure-boot \
  --shielded-vtpm \
  --shielded-integrity-monitoring

echo "GPU VM created at ${public_ip}. Driver setup can reboot it twice and take 10-20 minutes."
echo "This VM now incurs charges until stopped or deleted."
