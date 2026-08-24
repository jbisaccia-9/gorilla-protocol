#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud
"${SCRIPT_DIR}/preflight.sh"

if ! resource_exists gcloud compute networks describe "${GCP_NETWORK}" --project="${GCP_PROJECT_ID}"; then
  gcloud compute networks create "${GCP_NETWORK}" \
    --project="${GCP_PROJECT_ID}" --subnet-mode=custom --bgp-routing-mode=regional
fi

if ! resource_exists gcloud compute networks subnets describe "${GCP_SUBNET}" --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"; then
  gcloud compute networks subnets create "${GCP_SUBNET}" \
    --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" \
    --network="${GCP_NETWORK}" --range="${GCP_SUBNET_RANGE}" --enable-private-ip-google-access
else
  existing_subnet="$(gcloud compute networks subnets describe "${GCP_SUBNET}" \
    --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" \
    --format='value(network.basename(),ipCidrRange)')"
  [[ "${existing_subnet}" == "${GCP_NETWORK}"$'\t'"${GCP_SUBNET_RANGE}" ]] || die "Existing subnet configuration does not match gcp.env."
fi

create_firewall() {
  local name="$1"
  local rules="$2"
  local source_ranges="$3"
  local target_tags="$4"

  if ! resource_exists gcloud compute firewall-rules describe "${name}" --project="${GCP_PROJECT_ID}"; then
    gcloud compute firewall-rules create "${name}" \
      --project="${GCP_PROJECT_ID}" --network="${GCP_NETWORK}" \
      --direction=INGRESS --action=ALLOW --rules="${rules}" \
      --source-ranges="${source_ranges}" --target-tags="${target_tags}"
  else
    existing_network="$(gcloud compute firewall-rules describe "${name}" \
      --project="${GCP_PROJECT_ID}" --format='value(network.basename())')"
    [[ "${existing_network}" == "${GCP_NETWORK}" ]] || die "Firewall rule ${name} belongs to another network."
  fi
}

create_firewall gorilla-stream-web tcp:80,tcp:443 0.0.0.0/0 gorilla-stream-edge
create_firewall gorilla-stream-turn tcp:3478,udp:3478 0.0.0.0/0 gorilla-stream-edge
create_firewall gorilla-stream-media udp:49160-49200 0.0.0.0/0 gorilla-stream-edge
create_firewall gorilla-stream-iap-ssh tcp:22 35.235.240.0/20 gorilla-iap-ssh

if ! resource_exists gcloud compute addresses describe "${GCP_ADDRESS_NAME}" --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}"; then
  gcloud compute addresses create "${GCP_ADDRESS_NAME}" \
    --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" --network-tier=PREMIUM
fi

public_ip="$(gcloud compute addresses describe "${GCP_ADDRESS_NAME}" \
  --project="${GCP_PROJECT_ID}" --region="${GCP_REGION}" --format='value(address)')"
echo "Foundation ready. Create an A record for ${STREAM_DOMAIN:-play.example.com} pointing to ${public_ip}."
echo "The reserved address can incur charges while unused."
