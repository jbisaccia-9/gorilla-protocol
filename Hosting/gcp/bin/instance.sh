#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_tools gcloud

action="${1:-status}"
case "${action}" in
  status)
    gcloud compute instances describe "${GCP_VM_NAME}" \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" \
      --format='table(name,status,machineType.basename(),networkInterfaces[0].accessConfigs[0].natIP)'
    ;;
  stop)
    gcloud compute instances stop "${GCP_VM_NAME}" \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
    echo "VM stopped. Disk and reserved IP charges continue."
    ;;
  start)
    require_apply "${2:-}"
    gcloud compute instances start "${GCP_VM_NAME}" \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}"
    echo "VM started and compute charges resumed."
    ;;
  logs)
    gcloud compute ssh "${GCP_VM_NAME}" \
      --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap \
      --command='sudo tail -n 200 /var/log/gorilla-bootstrap.log'
    ;;
  *)
    die "Usage: instance.sh status|stop|start --apply|logs"
    ;;
esac
