#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
UAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"

test -x "${UAT}" || { echo "RunUAT.sh not found under UE_ROOT." >&2; exit 1; }
test -s "${ROOT}/Content/GorillaProtocol/Maps/L_Boot.umap" || {
  echo "Missing boot map. Run ./Scripts/bootstrap_project.sh first." >&2
  exit 1
}

"${UAT}" BuildCookRun \
  -project="${PROJECT}" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -compressed -prereqs -archive \
  -archivedirectory="${ROOT}/Artifacts/Linux-Shipping" -utf8output
