#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
UAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"
ARCHIVE_DIR="${ROOT}/Artifacts/Linux-Shipping"
PACKAGE_DIR="${ARCHIVE_DIR}/Linux"
OUTPUT_ARCHIVE="${ROOT}/Artifacts/GorillaProtocol-Linux.tar.gz"

test -x "${UAT}" || { echo "RunUAT.sh not found under UE_ROOT." >&2; exit 1; }
command -v tar >/dev/null || { echo "tar is required." >&2; exit 1; }
test -s "${ROOT}/Content/GorillaProtocol/Maps/L_Boot.umap" || {
  echo "Missing boot map. Run ./Scripts/bootstrap_project.sh first." >&2
  exit 1
}

"${UAT}" BuildCookRun \
  -project="${PROJECT}" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -compressed -prereqs -archive \
  -archivedirectory="${ARCHIVE_DIR}" -utf8output

test -x "${PACKAGE_DIR}/GorillaProtocol.sh" || {
  echo "Packaged game launcher not found: ${PACKAGE_DIR}/GorillaProtocol.sh" >&2
  exit 1
}

tar -C "${PACKAGE_DIR}" -czf "${OUTPUT_ARCHIVE}" .
echo "Cloud upload archive created: ${OUTPUT_ARCHIVE}"
