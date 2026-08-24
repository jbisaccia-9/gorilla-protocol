#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
PROJECT="$(cd "$(dirname "$0")/.." && pwd)/GorillaProtocol.uproject"
EDITOR="${UE_ROOT}/Engine/Binaries/Mac/UnrealEditor-Cmd"
REPORTS="$(cd "$(dirname "$0")/.." && pwd)/Tests/Reports"
mkdir -p "${REPORTS}"

"${EDITOR}" "${PROJECT}" \
  -unattended -nop4 -nosplash -nullrhi \
  -ExecCmds="Automation RunTests GorillaProtocol.Smoke;Quit" \
  -TestExit="Automation Test Queue Empty" \
  -ReportOutputPath="${REPORTS}"
