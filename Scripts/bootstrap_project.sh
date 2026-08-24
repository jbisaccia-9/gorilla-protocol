#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
EDITOR="${UE_ROOT}/Engine/Binaries/Mac/UnrealEditor"

"${EDITOR}" "${PROJECT}" -unattended -nop4 -nosplash \
  -ExecutePythonScript="${ROOT}/Scripts/create_boot_map.py"

test -s "${ROOT}/Content/GorillaProtocol/Maps/L_Boot.umap"
echo "Boot map created. Commit the generated L_Boot.umap through Git LFS."
