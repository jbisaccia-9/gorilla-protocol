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
BOOTSTRAP_LOG="$ROOT/Saved/Logs/PlayableBootstrap.log"
mkdir -p "$(dirname "$BOOTSTRAP_LOG")"

if ! "$EDITOR" "$ROOT/GorillaProtocol.uproject" \
  -ExecutePythonScript="$ROOT/Scripts/Unreal/bootstrap_playable.py" \
  -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput \
  2>&1 | tee "$BOOTSTRAP_LOG"; then
  echo "Unreal Editor failed while creating playable content." >&2
  exit 1
fi

if ! grep -q "PLAYABLE_BOOTSTRAP_COMPLETE" "$BOOTSTRAP_LOG"; then
  echo "Playable bootstrap failed. Relevant Unreal errors:" >&2
  grep -Ei "LogPython: Error|Traceback|RuntimeError|Exception" "$BOOTSTRAP_LOG" | tail -80 >&2 || true
  exit 1
fi

test -s "$ROOT/Content/GorillaProtocol/Maps/L_ScimmiaDiMare.umap" || {
  echo "Playable bootstrap did not create L_ScimmiaDiMare.umap." >&2
  exit 1
}

echo "Playable content created and saved in L_ScimmiaDiMare.umap."
