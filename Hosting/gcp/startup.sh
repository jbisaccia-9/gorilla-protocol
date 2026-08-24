#!/usr/bin/env bash
set -euo pipefail

exec > >(tee -a /var/log/gorilla-bootstrap.log) 2>&1
export DEBIAN_FRONTEND=noninteractive

install -d -m 755 /var/lib/gorilla /opt/gorilla-protocol
apt-get update
apt-get install -y ca-certificates curl git gnupg python3 build-essential linux-headers-"$(uname -r)" libvulkan1

if ! command -v docker >/dev/null; then
  install -m 0755 -d /etc/apt/keyrings
  curl -fsSL https://download.docker.com/linux/ubuntu/gpg -o /etc/apt/keyrings/docker.asc
  chmod a+r /etc/apt/keyrings/docker.asc
  . /etc/os-release
  echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.asc] https://download.docker.com/linux/ubuntu ${UBUNTU_CODENAME:-$VERSION_CODENAME} stable" > /etc/apt/sources.list.d/docker.list
  apt-get update
  apt-get install -y docker-ce docker-ce-cli containerd.io docker-buildx-plugin docker-compose-plugin
fi
systemctl enable --now docker

if ! nvidia-smi >/dev/null 2>&1; then
  driver_attempts="$(cat /var/lib/gorilla/driver-install-attempts 2>/dev/null || echo 0)"
  [[ "${driver_attempts}" =~ ^[0-9]+$ ]] || driver_attempts=0
  if (( driver_attempts >= 3 )); then
    echo "NVIDIA driver is unavailable after three installation attempts. Inspect this log." >&2
    exit 1
  fi

  driver_attempts=$((driver_attempts + 1))
  echo "${driver_attempts}" > /var/lib/gorilla/driver-install-attempts
  install -d -m 755 /opt/google/cuda-installer
  cd /opt/google/cuda-installer
  curl -fSsL -O https://storage.googleapis.com/compute-gpu-installation-us/installer/latest/cuda_installer.pyz
  python3 cuda_installer.pyz install_driver
  reboot
  exit 0
fi

if [[ ! -e /var/lib/gorilla/gsp-disabled ]]; then
  echo 'options nvidia NVreg_EnableGpuFirmware=0' > /etc/modprobe.d/nvidia-gsp.conf
  update-initramfs -u
  touch /var/lib/gorilla/gsp-disabled
  reboot
  exit 0
fi

if [[ ! -d /opt/gorilla-protocol/.git ]]; then
  rm -rf /opt/gorilla-protocol
  git clone --depth 1 https://github.com/jbisaccia-9/gorilla-protocol.git /opt/gorilla-protocol
fi

user_name="$(getent passwd 1000 | cut -d: -f1 || true)"
if [[ -n "${user_name}" ]]; then
  usermod -aG docker "${user_name}"
fi

nvidia-smi
nvidia_details="$(nvidia-smi -q)"
grep -A 3 'vGPU Software Licensed Product' <<<"${nvidia_details}" || true
grep -Eq 'License Status[[:space:]]*:[[:space:]]*Licensed' <<<"${nvidia_details}" || {
  echo "The NVIDIA RTX Virtual Workstation license is not active." >&2
  exit 1
}
touch /var/lib/gorilla/bootstrap-ready
echo "Gorilla Protocol host prerequisites are ready."
