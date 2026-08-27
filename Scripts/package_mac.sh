#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
UAT="${UE_ROOT}/Engine/Build/BatchFiles/RunUAT.sh"

"${ROOT}/Scripts/validate_vertical_slice.sh"

"${UAT}" BuildCookRun \
  -project="${PROJECT}" -platform=Mac -clientconfig=Development \
  -build -cook -stage -pak -iostore -compressed -archive \
  -archivedirectory="${ROOT}/Artifacts/Mac-Development" -utf8output
