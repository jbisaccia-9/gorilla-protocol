#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"

case "$(uname -s)" in
  Darwin) platform="Mac" ; editor_relative="Engine/Binaries/Mac/UnrealEditor-Cmd" ;;
  Linux) platform="Linux" ; editor_relative="Engine/Binaries/Linux/UnrealEditor-Cmd" ;;
  *) echo "Run playable bootstrap from Linux or macOS." >&2; exit 1 ;;
esac

source "$ROOT/Scripts/resolve_unreal_root.sh"
resolve_unreal_root "$platform"

EDITOR="$UE_ROOT/$editor_relative"
test -x "$EDITOR" || { echo "UnrealEditor-Cmd not found: $EDITOR" >&2; exit 1; }

"$ROOT/Scripts/validate_project.sh"
"$EDITOR" "$ROOT/GorillaProtocol.uproject" \
  -ExecutePythonScript="$ROOT/Scripts/Unreal/bootstrap_playable.py" \
  -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput

test -s "$ROOT/Content/GorillaProtocol/Maps/L_ScimmiaDiMare.umap" || {
  echo "Playable bootstrap did not create L_ScimmiaDiMare.umap." >&2
  exit 1
}

echo "Playable content created and saved in L_ScimmiaDiMare.umap."
