#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "Run this command on the Linux Unreal workstation." >&2
  exit 1
fi

ROOT="$(cd "$(dirname "$0")/.." && pwd -P)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
source "${ROOT}/Scripts/resolve_unreal_root.sh"
resolve_unreal_root Linux

"${ROOT}/Scripts/validate_project.sh"
"${ROOT}/Scripts/build_editor.sh"
"${ROOT}/Scripts/bootstrap_playable.sh"
"${ROOT}/Scripts/validate_vertical_slice.sh"

EDITOR="${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor"
test -x "${EDITOR}" || { echo "UnrealEditor not found: ${EDITOR}" >&2; exit 1; }

echo "Build complete. Starting Operazione Scimmia di Mare..."
exec "${EDITOR}" "${PROJECT}" /Game/GorillaProtocol/Maps/L_ScimmiaDiMare \
  -game -windowed -ResX=1600 -ResY=900 -log
