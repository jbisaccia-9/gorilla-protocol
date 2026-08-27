#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PROJECT="${ROOT}/GorillaProtocol.uproject"

case "$(uname -s)" in
  Darwin) platform="Mac" ; editor_relative="Engine/Binaries/Mac/UnrealEditor-Cmd" ;;
  Linux) platform="Linux" ; editor_relative="Engine/Binaries/Linux/UnrealEditor-Cmd" ;;
  *) echo "Run Data Validation from Unreal Editor on Windows." >&2; exit 1 ;;
esac

source "${ROOT}/Scripts/resolve_unreal_root.sh"
resolve_unreal_root "$platform"
EDITOR="${UE_ROOT}/${editor_relative}"
test -x "$EDITOR" || { echo "UnrealEditor-Cmd not found: ${EDITOR}" >&2; exit 1; }

"${EDITOR}" "${PROJECT}" -run=DataValidation -unattended -nop4 -nosplash -nullrhi
