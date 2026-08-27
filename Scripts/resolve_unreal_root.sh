#!/usr/bin/env bash

resolve_unreal_root() {
  local platform="$1"
  local build_relative="Engine/Build/BatchFiles/${platform}/Build.sh"
  local candidate=""
  local detected=""
  local candidates=(
    "${UE_ROOT:-}"
    "$HOME/Unreal/UE_5.8"
    "$HOME/UnrealEngine"
    "$HOME/UnrealEngine-5.8"
    "$HOME/Epic/UE_5.8"
    "/opt/UnrealEngine"
    "/opt/UnrealEngine/UE_5.8"
  )

  for candidate in "${candidates[@]}"; do
    [[ -n "$candidate" ]] || continue
    if [[ -f "$candidate/$build_relative" ]]; then
      UE_ROOT="$(cd -- "$candidate" && pwd -P)"
      export UE_ROOT
      return 0
    fi
    if [[ -f "$candidate/Build/BatchFiles/${platform}/Build.sh" ]]; then
      UE_ROOT="$(cd -- "$candidate/.." && pwd -P)"
      export UE_ROOT
      return 0
    fi
  done

  detected="$(find "$HOME" -maxdepth 7 -type f -path "*/$build_relative" -print -quit 2>/dev/null || true)"
  if [[ -n "$detected" ]]; then
    UE_ROOT="${detected%/$build_relative}"
    export UE_ROOT
    return 0
  fi

  printf 'Unreal Engine was not found. Expected this file under the Engine installation:\n' >&2
  printf '  %s\n\n' "$build_relative" >&2
  printf 'Set the actual extracted Engine directory and retry:\n' >&2
  printf '  export UE_ROOT="/absolute/path/to/UE_5.8"\n' >&2
  printf '  %s\n' "$0" >&2
  return 1
}
