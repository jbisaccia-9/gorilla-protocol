#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

python3 -m json.tool GorillaProtocol.uproject >/dev/null

required=(
  GorillaProtocol.uproject
  LINUX_BUILD_README.md
  Config/DefaultEngine.ini
  Scripts/build_linux_shipping.sh
  Source/GorillaProtocol.Target.cs
  Source/GorillaProtocolEditor.Target.cs
  Source/GorillaProtocol/GorillaProtocol.Build.cs
  Source/GorillaProtocol/Game/GPGameMode.cpp
  Source/GorillaProtocol/Characters/GPAgentCharacter.cpp
  Source/GorillaProtocol/AI/GPGuardAIController.cpp
  Hosting/docker-compose.yml
  Hosting/bin/validate_hosting.sh
  Licenses/AssetManifest.csv
)

for path in "${required[@]}"; do
  test -s "${path}" || { echo "Missing required file: ${path}" >&2; exit 1; }
done

if grep -RqIE --exclude-dir=.git --exclude='validate_project.sh' \
  '(AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|github_pat_|-----BEGIN .*PRIVATE KEY|sk-[A-Za-z0-9_-]{20,})' .; then
  echo "Potential credential found." >&2
  exit 1
fi

if grep -RqIE --include='*.cpp' --include='*.h' \
  '(FHttpModule|IHttpRequest|WebSocket|TcpSocket|UdpSocket)' Source; then
  echo "Unexpected runtime networking API found." >&2
  exit 1
fi

if find . -type f \( -iname '*.p12' -o -iname '*.pfx' -o -iname '*.pem' \
  -o -iname '*.key' -o -iname '*.mobileprovision' \) -print -quit | grep -q .; then
  echo "Certificate, private key, or provisioning profile found." >&2
  exit 1
fi

while IFS= read -r asset; do
  asset="${asset#./}"
  if ! grep -Fq "${asset}," Licenses/AssetManifest.csv; then
    echo "Binary asset missing from Licenses/AssetManifest.csv: ${asset}" >&2
    exit 1
  fi
done < <(find Content -type f \( -iname '*.uasset' -o -iname '*.umap' -o -iname '*.fbx' \
  -o -iname '*.wav' -o -iname '*.exr' \) -print)

if [[ ! -s Content/GorillaProtocol/Maps/L_Boot.umap ]]; then
  echo "Notice: L_Boot.umap has not been generated; run Scripts/bootstrap_project.sh after installing UE5.8." >&2
fi

./Hosting/bin/validate_hosting.sh

echo "Gorilla Protocol source validation passed."
