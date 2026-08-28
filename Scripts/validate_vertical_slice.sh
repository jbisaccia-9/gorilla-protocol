#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONTRACT="${ROOT}/Build/VerticalSliceAssets.txt"
MANIFEST="${ROOT}/Licenses/AssetManifest.csv"

test -s "${CONTRACT}" || { echo "Missing vertical-slice asset contract." >&2; exit 1; }
test -s "${MANIFEST}" || { echo "Missing asset license manifest." >&2; exit 1; }

missing=0
while IFS= read -r asset; do
  [[ -z "${asset}" || "${asset}" == \#* ]] && continue
  if [[ ! -s "${ROOT}/${asset}" ]]; then
    echo "Missing production asset: ${asset}" >&2
    missing=1
    continue
  fi
  if ! grep -Fq "${asset}," "${MANIFEST}"; then
    echo "Production asset is not licensed in AssetManifest.csv: ${asset}" >&2
    missing=1
  fi
done <"${CONTRACT}"

if (( missing != 0 )); then
  echo "Vertical slice is not packageable. Finish authored content; do not add proxy fallbacks." >&2
  exit 1
fi

echo "Gorilla Protocol vertical-slice asset gate passed."
