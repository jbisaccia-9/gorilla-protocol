#!/usr/bin/env bash
set -euo pipefail

RESET_ROOT="$(cd "$(dirname "$0")/../.." && pwd -P)"
source "$RESET_ROOT/Scripts/Lyra/resolve_lyra_root.sh"
resolve_lyra_root

PROJECT="$LYRA_ROOT/LyraStarterGame.uproject"
required=(
  "$PROJECT"
  "$LYRA_ROOT/Source/LyraEditor.Target.cs"
  "$LYRA_ROOT/Plugins/GameFeatures/ShooterCore/ShooterCore.uplugin"
  "$LYRA_ROOT/Plugins/GameFeatures/ShooterCore/Content/Maps/L_ShooterGym.umap"
  "$LYRA_ROOT/Plugins/GameFeatures/ShooterMaps/ShooterMaps.uplugin"
  "$LYRA_ROOT/Plugins/GameFeatures/ShooterMaps/Content/Maps/L_Expanse.umap"
)

for path in "${required[@]}"; do
  [[ -s "$path" ]] || {
    echo "Lyra installation is incomplete; missing: $path" >&2
    exit 1
  }
done

python3 -m json.tool "$PROJECT" >/dev/null

credential_pattern='(AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|github_pat_|-----BEGIN .*PRIVATE KEY|sk-[A-Za-z0-9_-]{20,})'
if command -v rg >/dev/null 2>&1; then
  if (cd "$RESET_ROOT" && rg -n --hidden \
    -g '*.ini' -g '*.json' -g '*.cs' -g '*.sh' -g '*.md' \
    -g '!.git/**' -g '!**/validate*.sh' \
    "$credential_pattern" .); then
    echo "Potential credential found in the Gorilla reset source." >&2
    exit 1
  fi
fi

echo "Lyra baseline validation passed: $LYRA_ROOT"
