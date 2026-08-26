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

if [[ "$(git -C /opt/gorilla-protocol rev-parse --is-shallow-repository)" == "true" ]]; then
  git -C /opt/gorilla-protocol fetch --unshallow origin
else
  git -C /opt/gorilla-protocol fetch origin main
fi
git -C /opt/gorilla-protocol checkout --detach origin/main
/opt/gorilla-protocol/Hosting/bin/install_host_services.sh
install -o root -g gorilla-stream -m 640 \
  "${env_file}" /opt/gorilla-protocol/Hosting/.env
/opt/gorilla-protocol/Hosting/bin/start_stack.sh
systemctl restart gorilla-streamer.service

echo "Release activated."
