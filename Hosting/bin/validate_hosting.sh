#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT_DIR="$(cd "${HOSTING_DIR}/.." && pwd)"

required=(
  "${HOSTING_DIR}/docker-compose.yml"
  "${HOSTING_DIR}/Caddyfile"
  "${HOSTING_DIR}/.env.example"
  "${HOSTING_DIR}/templates/peer-options.json.tpl"
  "${HOSTING_DIR}/templates/turnserver.conf.tpl"
  "${HOSTING_DIR}/bin/fetch_epic_infrastructure.sh"
  "${HOSTING_DIR}/bin/prepare_host.sh"
  "${HOSTING_DIR}/bin/start_stack.sh"
  "${HOSTING_DIR}/bin/start_streamer_linux.sh"
  "${HOSTING_DIR}/gcp/gcp.env.example"
  "${HOSTING_DIR}/gcp/startup.sh"
  "${HOSTING_DIR}/gcp/lib/common.sh"
  "${HOSTING_DIR}/gcp/bin/preflight.sh"
  "${HOSTING_DIR}/gcp/bin/create_foundation.sh"
  "${HOSTING_DIR}/gcp/bin/create_gpu_vm.sh"
  "${HOSTING_DIR}/gcp/bin/destroy.sh"
)

for path in "${required[@]}"; do
  test -s "${path}" || { echo "Missing hosting file: ${path}" >&2; exit 1; }
done

while IFS= read -r script; do
  bash -n "${script}"
done < <(find "${HOSTING_DIR}/bin" "${HOSTING_DIR}/gcp" -type f -name '*.sh' -print)

python3 - "${PROJECT_DIR}/GorillaProtocol.uproject" <<'PY'
import json
import sys

with open(sys.argv[1], encoding="utf-8") as handle:
    project = json.load(handle)

plugins = {item["Name"]: item.get("Enabled", False) for item in project["Plugins"]}
if not plugins.get("PixelStreaming2"):
    raise SystemExit("PixelStreaming2 must be enabled in GorillaProtocol.uproject")
PY

if find "${HOSTING_DIR}" -type f \( -name '.env' -o -name 'turn-secret' -o -name '*.pem' -o -name '*.key' \) \
  -not -path "${HOSTING_DIR}/runtime/*" -print -quit | grep -q .; then
  echo "A generated credential or local environment file is outside Hosting/runtime." >&2
  exit 1
fi

echo "Gorilla Protocol hosting validation passed."
