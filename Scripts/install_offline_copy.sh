#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"
SOURCE_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd -P)"
DEST_ROOT="${1:-$HOME/Projects/gorilla-protocol}"

if [[ ! -f "$SOURCE_ROOT/GorillaProtocol.uproject" ]]; then
  printf 'Source checkout is incomplete: %s\n' "$SOURCE_ROOT" >&2
  exit 1
fi

if [[ -d "$DEST_ROOT" ]] && [[ -n "$(find "$DEST_ROOT" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
  printf 'Destination already exists and is not empty: %s\n' "$DEST_ROOT" >&2
  printf 'Choose an empty path: bash %q /path/to/destination\n' "$0" >&2
  exit 1
fi

mkdir -p "$DEST_ROOT"
(
  cd "$SOURCE_ROOT"
  env COPYFILE_DISABLE=1 tar \
    --exclude='._*' \
    --exclude='*/._*' \
    --exclude='.DS_Store' \
    --exclude='*/.DS_Store' \
    -cf - .
) | (
  cd "$DEST_ROOT"
  env COPYFILE_DISABLE=1 tar -xf -
)

cd "$DEST_ROOT"
if command -v git-lfs >/dev/null 2>&1; then
  git lfs install --local
  git lfs checkout
fi

./Scripts/validate_project.sh

printf '\nOffline checkout installed at:\n  %s\n' "$DEST_ROOT"
printf 'Next: export UE_ROOT="$HOME/Unreal/UE_5.8"\n'
printf 'Then: cd %q && ./Scripts/build_editor.sh\n' "$DEST_ROOT"
