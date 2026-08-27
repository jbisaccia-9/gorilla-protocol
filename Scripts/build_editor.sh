#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
PROJECT="$(cd "$(dirname "$0")/.." && pwd)/GorillaProtocol.uproject"

case "$(uname -s)" in
  Darwin) platform="Mac" ;;
  Linux) platform="Linux" ;;
  *) echo "Use Unreal's generated Visual Studio project on Windows." >&2; exit 1 ;;
esac

"${UE_ROOT}/Engine/Build/BatchFiles/${platform}/Build.sh" \
  GorillaProtocolEditor "${platform}" Development "${PROJECT}" -WaitMutex
