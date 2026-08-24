#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_tools gcloud awk

active_account="$(gcloud auth list --filter=status:ACTIVE --limit=1 --format='value(account)')"
test -n "${active_account}" || die "Run gcloud auth login first."

gcloud projects describe "${GCP_PROJECT_ID}" --format='value(projectId)' >/dev/null
billing_enabled="$(gcloud billing projects describe "${GCP_PROJECT_ID}" --format='value(billingEnabled)')"
case "${billing_enabled}" in
  true|True|TRUE) ;;
  *) die "Billing is not enabled for ${GCP_PROJECT_ID}." ;;
esac

for required_service in compute.googleapis.com iap.googleapis.com oslogin.googleapis.com; do
  enabled_service="$(gcloud services list --enabled \
    --project="${GCP_PROJECT_ID}" \
    --filter="config.name:${required_service}" \
    --format='value(config.name)')"
  [[ "${enabled_service}" == "${required_service}" ]] || die "Enable ${required_service} first."
done

gcloud compute machine-types describe "${GCP_MACHINE_TYPE}" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" >/dev/null
gcloud compute accelerator-types describe "${GCP_ACCELERATOR_TYPE}" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" >/dev/null

read -r global_limit global_usage < <(quota_record global GPUS_ALL_REGIONS)
regional_metric="NVIDIA_L4_GPUS"
if [[ "${GCP_ACCELERATOR_TYPE}" == "nvidia-l4-vws" ]]; then
  regional_metric="NVIDIA_L4_VWS_GPUS"
fi
read -r regional_limit regional_usage < <(quota_record regional "${regional_metric}")

echo "Project: ${GCP_PROJECT_ID}"
echo "Target: ${GCP_MACHINE_TYPE} with ${GCP_ACCELERATOR_TYPE} in ${GCP_ZONE}"
echo "Global GPU quota: ${global_usage}/${global_limit}"
echo "${regional_metric} quota in ${GCP_REGION}: ${regional_usage}/${regional_limit}"

awk -v limit="${global_limit}" -v usage="${global_usage}" 'BEGIN { exit !((limit - usage) >= 1) }' || {
  echo "Request GPUs (all regions) quota of at least 1:" >&2
  echo "https://console.cloud.google.com/iam-admin/quotas?project=${GCP_PROJECT_ID}" >&2
  exit 2
}

awk -v limit="${regional_limit}" -v usage="${regional_usage}" 'BEGIN { exit !((limit - usage) >= 1) }' || {
  echo "Request ${regional_metric} quota of at least 1 in ${GCP_REGION}:" >&2
  echo "https://console.cloud.google.com/iam-admin/quotas?project=${GCP_PROJECT_ID}" >&2
  exit 2
}

echo "Preflight passed. No resources were created or modified."
