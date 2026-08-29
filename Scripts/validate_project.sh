#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "${ROOT}"

python3 -m json.tool GorillaProtocol.uproject >/dev/null

required=(
  GorillaProtocol.uproject
  LINUX_BUILD_README.md
  Config/DefaultEngine.ini
  Build/VerticalSliceAssets.txt
  Scripts/build_linux_shipping.sh
  Scripts/validate_vertical_slice.sh
  Source/GorillaProtocol.Target.cs
  Source/GorillaProtocolEditor.Target.cs
  Source/GorillaProtocol/GorillaProtocol.Build.cs
  Source/GorillaProtocol/Core/GPVerticalSliceDefinition.cpp
  Source/GorillaProtocol/Game/GPGameModeBase.cpp
  Source/GorillaProtocol/Player/GPBrunoCharacter.cpp
  Source/GorillaProtocol/AI/GPGuardCharacter.cpp
  Source/GorillaProtocol/Game/GPHUD.cpp
  Source/GorillaProtocol/Game/GPMissionZone.cpp
  Scripts/Unreal/bootstrap_playable.py
  Hosting/docker-compose.yml
  Hosting/bin/validate_hosting.sh
  Licenses/AssetManifest.csv
)

for path in "${required[@]}"; do
  test -s "${path}" || { echo "Missing required file: ${path}" >&2; exit 1; }
done

while IFS= read -r script; do
  bash -n "${script}"
done < <(find Scripts -type f -name '*.sh' -print)

credential_pattern='(AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|github_pat_|-----BEGIN .*PRIVATE KEY|sk-[A-Za-z0-9_-]{20,})'
if command -v rg >/dev/null 2>&1; then
  if rg -n --hidden \
    -g '!.git/**' -g '!RawContent/**' -g '!Content/**' -g '!archive/**' \
    -g '*.cpp' -g '*.h' -g '*.cs' -g '*.py' -g '*.sh' -g '*.ps1' \
    -g '*.ini' -g '*.json' -g '*.yml' -g '*.yaml' -g '*.md' -g '*.csv' \
    -g '!**/validate*.sh' \
    "${credential_pattern}" .; then
    echo "Potential credential found." >&2
    exit 1
  fi
else
  while IFS= read -r file; do
    if grep -Eq "${credential_pattern}" "${file}"; then
      echo "Potential credential found: ${file}" >&2
      exit 1
    fi
  done < <(find . -type f \
    \( -name '*.cpp' -o -name '*.h' -o -name '*.cs' -o -name '*.py' -o -name '*.sh' \
    -o -name '*.ps1' -o -name '*.ini' -o -name '*.json' -o -name '*.yml' \
    -o -name '*.yaml' -o -name '*.md' -o -name '*.csv' \) \
    -not -path './.git/*' -not -path './RawContent/*' -not -path './Content/*' \
    -not -path './archive/*' -not -path './Scripts/validate*.sh')
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
    echo "Raw asset missing from Licenses/AssetManifest.csv: ${asset}" >&2
    exit 1
  fi
done < <(find RawContent -type f \( -iname '*.blend' -o -iname '*.fbx' -o -iname '*.wav' \
  -o -iname '*.exr' -o -iname '*.png' -o -iname '*.jpg' \) -print)

./Hosting/bin/validate_hosting.sh

echo "Gorilla Protocol source validation passed."
