#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=../lib/common.sh
. "${SCRIPT_DIR}/../lib/common.sh"

load_config
require_apply "${1:-}"
require_tools gcloud tar

archive="${2:-}"
test -f "${archive}" || die "Usage: upload_package.sh --apply /path/to/GorillaProtocol-Linux.tar.gz"

if grep -Eq '(^/|(^|/)\.\.(/|$))' < <(tar -tzf "${archive}"); then
  die "Archive contains an unsafe absolute or parent path."
fi

gcloud compute scp "${archive}" "${GCP_VM_NAME}:/tmp/gorilla-package.tar.gz" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap

gcloud compute ssh "${GCP_VM_NAME}" \
  --project="${GCP_PROJECT_ID}" --zone="${GCP_ZONE}" --tunnel-through-iap \
  --command='sudo install -d -m 755 /opt/gorilla-game && sudo find /opt/gorilla-game -mindepth 1 -maxdepth 1 -exec rm -rf -- {} + && sudo tar -xzf /tmp/gorilla-package.tar.gz -C /opt/gorilla-game && sudo rm -f /tmp/gorilla-package.tar.gz'

echo "Package uploaded. Confirm GAME_BINARY in gcp.env, then run deploy_host.sh --apply."
