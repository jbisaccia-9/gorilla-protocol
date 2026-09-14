#!/bin/bash
set -euo pipefail
PROJECT_DIR="$(cd -- "$(dirname -- "$0")" && pwd -P)"
BLENDER_BIN="${BLENDER_BIN:-/Applications/Blender.app/Contents/MacOS/Blender}"
if [[ ! -x "$BLENDER_BIN" ]]; then
  echo "Blender was not found. Set BLENDER_BIN to your Blender executable."
  read -r -p "Press Return to close. " _
  exit 1
fi
mkdir -p "$PROJECT_DIR/Art/CoastalSlice"
"$BLENDER_BIN" -b --python-exit-code 1 --python "$PROJECT_DIR/Scripts/Blender/build_coastal_slice.py" -- --preview \
  2>&1 | tee "$PROJECT_DIR/Art/CoastalSlice/build.log"
if [[ ! -s "$PROJECT_DIR/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend" ]]; then
  echo "Scene generation failed. See Art/CoastalSlice/build.log."
  exit 1
fi
open -a Blender "$PROJECT_DIR/Art/CoastalSlice/Gorilla_Coastal_Encounter.blend"
