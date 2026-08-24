#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud

gcloud projects describe "${GCP_PROJECT_ID}" --format='value(projectId)' >/dev/null
gcloud services enable \
  compute.googleapis.com \
  iap.googleapis.com \
  oslogin.googleapis.com \
  serviceusage.googleapis.com \
  --project="${GCP_PROJECT_ID}"

echo "Required Google Cloud services are enabled. No runtime resource was created."
