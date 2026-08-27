#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
PROJECT="$(cd "$SCRIPT_DIR/.." && pwd)/GorillaProtocol.uproject"

case "$(uname -s)" in
  Darwin) platform="Mac" ; editor_relative="Engine/Binaries/Mac/UnrealEditor" ;;
  Linux) platform="Linux" ; editor_relative="Engine/Binaries/Linux/UnrealEditor" ;;
  *) echo "Use Unreal's generated Visual Studio project on Windows." >&2; exit 1 ;;
esac

source "$SCRIPT_DIR/resolve_unreal_root.sh"
resolve_unreal_root "$platform"

EDITOR="$UE_ROOT/$editor_relative"
if [[ ! -x "$EDITOR" ]]; then
  printf 'Unreal Editor executable not found or not executable: %s\n' "$EDITOR" >&2
  exit 1
fi

exec "$EDITOR" "$PROJECT"
