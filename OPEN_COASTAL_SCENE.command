#!/bin/bash
set -euo pipefail
PROJECT_DIR="$(cd -- "$(dirname -- "$0")" && pwd -P)"
SCENE="$PROJECT_DIR/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend"
if [[ ! -s "$SCENE" ]]; then
  echo "The Blender scene has not been generated yet. Run BUILD_COASTAL_SCENE.command first."
  read -r -p "Press Return to close. " _
  exit 1
fi
open -a Blender "$SCENE"
