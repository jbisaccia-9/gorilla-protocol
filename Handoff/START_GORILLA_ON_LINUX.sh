#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd -P)"
ARCHIVE="${HERE}/GorillaProtocol-Source.tar.gz"
COMMIT_FILE="${HERE}/SOURCE_COMMIT.txt"

test -s "${ARCHIVE}" || { echo "Missing ${ARCHIVE}" >&2; exit 1; }
test -s "${COMMIT_FILE}" || { echo "Missing ${COMMIT_FILE}" >&2; exit 1; }

commit="$(tr -d '[:space:]' <"${COMMIT_FILE}")"
destination="${HOME}/Projects/gorilla-protocol-${commit:0:8}"

if [[ ! -d "${destination}/.git" ]]; then
  temporary="${destination}.installing.$$"
  trap 'rm -rf "${temporary}"' EXIT
  mkdir -p "${temporary}"
  tar -xzf "${ARCHIVE}" -C "${temporary}"
  mkdir -p "$(dirname "${destination}")"
  mv "${temporary}/gorilla-protocol" "${destination}"
  trap - EXIT
fi

actual_commit="$(git -C "${destination}" rev-parse HEAD)"
if [[ "${actual_commit}" != "${commit}" ]]; then
  echo "Installed source does not match the flash-drive commit." >&2
  echo "Expected: ${commit}" >&2
  echo "Found:    ${actual_commit}" >&2
  exit 1
fi

cd "${destination}"
git lfs install --local >/dev/null 2>&1 || true
git lfs checkout
exec ./Scripts/build_and_play_linux.sh
