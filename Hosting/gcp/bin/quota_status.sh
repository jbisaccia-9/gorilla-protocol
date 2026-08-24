#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_tools gcloud

for preference_id in gorilla-global-gpu-1 "gorilla-l4-vws-${GCP_REGION}-1"; do
  gcloud quotas preferences describe "${preference_id}" \
    --project="${GCP_PROJECT_ID}" \
    --billing-project="${GCP_PROJECT_ID}" \
    --format='table(name.basename(),quotaId,dimensions,quotaConfig.preferredValue,quotaConfig.grantedValue,reconciling)'
done
