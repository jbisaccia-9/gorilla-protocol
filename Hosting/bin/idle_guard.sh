#!/usr/bin/env bash
set -euo pipefail

HOSTING_DIR="$(cd "$(dirname "$0")/.." && pwd)"
ENV_FILE="${HOSTING_DIR}/.env"
STATE_DIR="/var/lib/gorilla/idle-guard"

if [[ -s "${ENV_FILE}" ]]; then
  set -a
  # shellcheck disable=SC1090
  . "${ENV_FILE}"
  set +a
fi

IDLE_SHUTDOWN_SECONDS="${IDLE_SHUTDOWN_SECONDS:-600}"
BOOT_GRACE_SECONDS="${BOOT_GRACE_SECONDS:-900}"
MAX_RUNTIME_SECONDS="${MAX_RUNTIME_SECONDS:-7200}"

for value in "${IDLE_SHUTDOWN_SECONDS}" "${BOOT_GRACE_SECONDS}" "${MAX_RUNTIME_SECONDS}"; do
  [[ "${value}" =~ ^[0-9]+$ ]] || {
    echo "Idle guard values must be integers." >&2
    exit 1
  }
done

mkdir -p "${STATE_DIR}"
boot_id="$(cat /proc/sys/kernel/random/boot_id)"
read -r uptime_seconds _ </proc/uptime
uptime_seconds="${uptime_seconds%.*}"
now="$(date +%s)"

if [[ "$(cat "${STATE_DIR}/boot-id" 2>/dev/null || true)" != "${boot_id}" ]]; then
  printf '%s\n' "${boot_id}" >"${STATE_DIR}/boot-id"
  printf '%s\n' "${now}" >"${STATE_DIR}/last-activity"
fi

if (( uptime_seconds >= MAX_RUNTIME_SECONDS )); then
  logger -t gorilla-idle-guard "Maximum runtime reached; powering off."
  systemctl poweroff
  exit 0
fi

if ss -Htn state established | awk '$4 ~ /:443$/ { found=1 } END { exit !found }'; then
  printf '%s\n' "${now}" >"${STATE_DIR}/last-activity"
  exit 0
fi

if (( uptime_seconds < BOOT_GRACE_SECONDS )); then
  exit 0
fi

last_activity="$(cat "${STATE_DIR}/last-activity" 2>/dev/null || printf '%s' "${now}")"
if (( now - last_activity >= IDLE_SHUTDOWN_SECONDS )); then
  logger -t gorilla-idle-guard "No player connection within the idle window; powering off."
  systemctl poweroff
fi
