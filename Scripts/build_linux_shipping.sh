#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine 5.8 installation directory}"

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "This build must run on Linux." >&2
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
SETUP_TOOLCHAIN="${UE_ROOT}/Engine/Build/BatchFiles/Linux/SetupToolchain.sh"
BUILD="${UE_ROOT}/Engine/Build/BatchFiles/Linux/Build.sh"
EDITOR="${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor"

for executable in "${SETUP_TOOLCHAIN}" "${BUILD}" "${EDITOR}"; do
  test -x "${executable}" || {
    echo "Required Unreal Engine executable not found: ${executable}" >&2
    exit 1
  }
done

command -v git >/dev/null || { echo "git is required." >&2; exit 1; }
command -v tar >/dev/null || { echo "tar is required." >&2; exit 1; }
command -v sha256sum >/dev/null || { echo "sha256sum is required." >&2; exit 1; }
git lfs version >/dev/null || { echo "Git LFS is required." >&2; exit 1; }

cd "${ROOT}"
git lfs pull
"${SETUP_TOOLCHAIN}"
"${BUILD}" GorillaProtocolEditor Linux Development "${PROJECT}" -WaitMutex
"${ROOT}/Scripts/bootstrap_project.sh"
"${ROOT}/Scripts/validate_project.sh"
"${ROOT}/Scripts/package_linux.sh"

ARCHIVE="${ROOT}/Artifacts/GorillaProtocol-Linux.tar.gz"
sha256sum "${ARCHIVE}" >"${ARCHIVE}.sha256"

echo "Linux Shipping archive ready: ${ARCHIVE}"
echo "Return this file for cloud deployment."
