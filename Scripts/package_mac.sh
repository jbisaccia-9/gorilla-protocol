#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
source "${ROOT}/Scripts/resolve_unreal_root.sh"
resolve_unreal_root Mac
UAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"

"${ROOT}/Scripts/validate_vertical_slice.sh"

bash "${UAT}" BuildCookRun \
  -project="${PROJECT}" -platform=Mac -clientconfig=Development \
  -build -cook -stage -pak -iostore -compressed -archive \
  -archivedirectory="${ROOT}/Artifacts/Mac-Development" -utf8output
