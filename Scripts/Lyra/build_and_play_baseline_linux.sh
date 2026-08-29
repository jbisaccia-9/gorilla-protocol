#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "Run the Lyra baseline on the Linux Unreal workstation." >&2
  exit 1
fi

RESET_ROOT="$(cd "$(dirname "$0")/../.." && pwd -P)"
source "$RESET_ROOT/Scripts/resolve_unreal_root.sh"
source "$RESET_ROOT/Scripts/Lyra/resolve_lyra_root.sh"

resolve_unreal_root Linux
resolve_lyra_root
"$RESET_ROOT/Scripts/Lyra/validate_lyra_baseline.sh"

PROJECT="$LYRA_ROOT/LyraStarterGame.uproject"
BUILD="$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
EDITOR="$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"

[[ -x "$BUILD" ]] || { echo "Unreal build tool not found: $BUILD" >&2; exit 1; }
[[ -x "$EDITOR" ]] || { echo "Unreal Editor not found: $EDITOR" >&2; exit 1; }

echo "Compiling the official Lyra editor target..."
"$BUILD" LyraEditor Linux Development "$PROJECT" -WaitMutex

echo "Starting Lyra Expanse. This is the mechanical quality baseline."
exec "$EDITOR" "$PROJECT" /ShooterMaps/Maps/L_Expanse \
  -game -windowed -ResX=1600 -ResY=900 -log \
  -ExecCmds="stat fps,stat unit"
