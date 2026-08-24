#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
INFRA_DIR="${PIXEL_STREAMING_INFRA_DIR:-${SCRIPT_DIR}/.cache/PixelStreamingInfrastructure}"
EPIC_REMOTE="https://github.com/EpicGames/PixelStreamingInfrastructure.git"
EPIC_COMMIT="d063f92e69750bc2eafd7e88011444cfddef1cbf"

if [[ -d "${INFRA_DIR}/.git" ]]; then
  current_commit="$(git -C "${INFRA_DIR}" rev-parse HEAD)"
  if [[ "${current_commit}" != "${EPIC_COMMIT}" ]]; then
    echo "Pixel Streaming infrastructure is not at the reviewed UE5.8 commit." >&2
    echo "Expected ${EPIC_COMMIT}; found ${current_commit}." >&2
    exit 1
  fi
  echo "Epic Pixel Streaming infrastructure is already pinned at ${EPIC_COMMIT}."
  exit 0
fi

test ! -e "${INFRA_DIR}" || {
  echo "Refusing to replace non-Git path: ${INFRA_DIR}" >&2
  exit 1
}

mkdir -p "$(dirname "${INFRA_DIR}")"
git init -q "${INFRA_DIR}"
git -C "${INFRA_DIR}" remote add origin "${EPIC_REMOTE}"
git -C "${INFRA_DIR}" fetch --depth 1 origin "${EPIC_COMMIT}"
git -C "${INFRA_DIR}" checkout -q --detach FETCH_HEAD

test "$(git -C "${INFRA_DIR}" rev-parse HEAD)" = "${EPIC_COMMIT}"
echo "Downloaded Epic Pixel Streaming UE5.8 infrastructure at ${EPIC_COMMIT}."
