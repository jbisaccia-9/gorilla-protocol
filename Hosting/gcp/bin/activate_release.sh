#!/usr/bin/env bash
set -euo pipefail

env_file="${1:-}"
test -s "${env_file}" || {
  echo "Usage: activate_release.sh /path/to/hosting.env" >&2
  exit 1
}

if (( EUID != 0 )); then
  echo "Run activate_release.sh as root." >&2
  exit 1
fi

git -C /opt/gorilla-protocol fetch --depth 1 origin main
git -C /opt/gorilla-protocol checkout --detach FETCH_HEAD
install -m 600 "${env_file}" /opt/gorilla-protocol/Hosting/.env
/opt/gorilla-protocol/Hosting/bin/start_stack.sh
/opt/gorilla-protocol/Hosting/bin/install_host_services.sh
systemctl restart gorilla-streamer.service

echo "Release activated."
