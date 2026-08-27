#!/usr/bin/env bash
set -euo pipefail

: "${UE_ROOT:?Set UE_ROOT to the Unreal Engine installation directory}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"

case "$(uname -s)" in
  Darwin) EDITOR="${UE_ROOT}/Engine/Binaries/Mac/UnrealEditor-Cmd" ;;
  Linux) EDITOR="${UE_ROOT}/Engine/Binaries/Linux/UnrealEditor-Cmd" ;;
  *) echo "Run Data Validation from Unreal Editor on Windows." >&2; exit 1 ;;
esac

"${EDITOR}" "${PROJECT}" -run=DataValidation -unattended -nop4 -nosplash -nullrhi
