#!/usr/bin/env bash

resolve_lyra_root() {
  local candidate=""
  local project=""
  local candidates=(
    "${LYRA_ROOT:-}"
    "$HOME/Projects/LyraStarterGame"
    "$HOME/Unreal/LyraStarterGame"
    "$HOME/Downloads/LyraStarterGame"
    "$HOME/Documents/Unreal Projects/LyraStarterGame"
  )

  for candidate in "${candidates[@]}"; do
    [[ -n "$candidate" ]] || continue
    if [[ -s "$candidate/LyraStarterGame.uproject" ]]; then
      LYRA_ROOT="$(cd -- "$candidate" && pwd -P)"
      export LYRA_ROOT
      return 0
    fi
  done

  project="$(find "$HOME" -maxdepth 7 -type f -name LyraStarterGame.uproject -print -quit 2>/dev/null || true)"
  if [[ -n "$project" ]]; then
    LYRA_ROOT="$(cd -- "$(dirname "$project")" && pwd -P)"
    export LYRA_ROOT
    return 0
  fi

  cat >&2 <<'EOF'
Lyra Starter Game was not found.

Acquire the official free Lyra Starter Game under your Epic account, then place
the complete project at one of these locations:
  $HOME/Projects/LyraStarterGame
  $HOME/Unreal/LyraStarterGame

If it is elsewhere, set its location before retrying:
  export LYRA_ROOT="/absolute/path/to/LyraStarterGame"
EOF
  return 1
}
