# Build The Linux Shipping Package

This workflow runs only after the authored vertical slice passes its content and
license gate. It no longer generates a placeholder map or substitutes proxy art.

## 1. Prepare The Linux Workstation

Use a Linux PC with at least 32 GB RAM, 200 GB free SSD space, a current Vulkan
driver, Git LFS, and the Epic Unreal Engine 5.8 Linux distribution.

```bash
sudo apt update
sudo apt install -y build-essential git git-lfs python3 unzip vulkan-tools
git lfs install
```

Extract Unreal Engine to `$HOME/Unreal/UE_5.8`, or adjust `UE_ROOT` below.

## 2. Get The Active Rebuild

```bash
mkdir -p "$HOME/Projects"
cd "$HOME/Projects"
git clone https://github.com/jbisaccia-9/gorilla-protocol.git
cd gorilla-protocol
git lfs pull
export UE_ROOT="$HOME/Unreal/UE_5.8"
```

For an existing checkout:

```bash
cd "$HOME/Projects/gorilla-protocol"
git pull --ff-only
git lfs pull
export UE_ROOT="$HOME/Unreal/UE_5.8"
```

## 3. Build And Author Content

```bash
./Scripts/build_editor.sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" GorillaProtocol.uproject
```

Author the map, character, animation, weapon, AI, UI, VFX, audio, and input assets
listed in `Build/VerticalSliceAssets.txt`. Add each committed asset to
`Licenses/AssetManifest.csv`. Do not proceed with proxy content.

Check readiness from Terminal:

```bash
./Scripts/validate_project.sh
./Scripts/validate_vertical_slice.sh
```

The second command is supposed to fail until all production content exists.

## 4. Build The Upload Archive

After local packaged gameplay meets `Docs/VERTICAL_SLICE.md`:

```bash
./Scripts/build_linux_shipping.sh
```

This command configures Epic's Linux toolchain, compiles the Editor target, runs
source and production-asset validation, cooks the authored map, packages Shipping,
and creates:

```text
$HOME/Projects/gorilla-protocol/Artifacts/GorillaProtocol-Linux.tar.gz
$HOME/Projects/gorilla-protocol/Artifacts/GorillaProtocol-Linux.tar.gz.sha256
```

`GORILLA_BUILD.txt` inside the archive records the exact Git commit, source state,
UE version, build time, and `content_gate=passed`. Cloud upload remains a separate
decision after local quality approval.

Common failures:

- `Missing production asset`: finish the named authored asset; do not bypass the gate.
- `not licensed`: add a complete `AssetManifest.csv` row and retain proof privately.
- `UnrealEditor not found`: correct `UE_ROOT`.
- Vulkan errors: update the GPU driver and run `vulkaninfo --summary`.
- Memory or disk errors: verify `free -h` and `df -h "$HOME"`.
