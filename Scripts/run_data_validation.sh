#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"
EDITOR="${UE_ROOT}/Engine/Binaries/Mac/UnrealEditor-Cmd"

"${EDITOR}" "${PROJECT}" -run=DataValidation -unattended -nop4 -nosplash -nullrhi
