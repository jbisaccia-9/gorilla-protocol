#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
source "${ROOT}/Scripts/resolve_unreal_root.sh"
resolve_unreal_root Linux
UAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"
ARCHIVE_DIR="${ROOT}/Artifacts/Linux-Shipping"
PACKAGE_DIR="${ARCHIVE_DIR}/Linux"
OUTPUT_ARCHIVE="${ROOT}/Artifacts/GorillaProtocol-Linux.tar.gz"

test -f "${UAT}" || { echo "RunUAT.sh not found under UE_ROOT: ${UAT}" >&2; exit 1; }
command -v tar >/dev/null || { echo "tar is required." >&2; exit 1; }
command -v git >/dev/null || { echo "git is required." >&2; exit 1; }
"${ROOT}/Scripts/validate_vertical_slice.sh"

bash "${UAT}" BuildCookRun \
  -project="${PROJECT}" -noP4 -platform=Linux -clientconfig=Shipping \
  -build -cook -stage -pak -iostore -compressed -prereqs -archive \
  -archivedirectory="${ARCHIVE_DIR}" -utf8output

test -x "${PACKAGE_DIR}/GorillaProtocol.sh" || {
  echo "Packaged game launcher not found: ${PACKAGE_DIR}/GorillaProtocol.sh" >&2
  exit 1
}

build_commit="$(git -C "${ROOT}" rev-parse HEAD)"
build_state="$(git -C "${ROOT}" describe --always --dirty)"
build_time="$(date -u '+%Y-%m-%dT%H:%M:%SZ')"
cat >"${PACKAGE_DIR}/GORILLA_BUILD.txt" <<EOF
project=Gorilla Protocol
engine=Unreal Engine 5.8
commit=${build_commit}
source_state=${build_state}
built_utc=${build_time}
content_gate=passed
EOF

tar -C "${PACKAGE_DIR}" -czf "${OUTPUT_ARCHIVE}" .
echo "Cloud upload archive created: ${OUTPUT_ARCHIVE}"
echo "Build manifest: ${build_state} at ${build_time}"
