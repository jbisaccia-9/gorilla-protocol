#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
PROJECT="$(cd "$(dirname "$0")/.." && pwd)/GorillaProtocol.uproject"

"${UE_ROOT}/Engine/Build/BatchFiles/Mac/Build.sh" \
  GorillaProtocolEditor Mac Development "${PROJECT}" -WaitMutex
