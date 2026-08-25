# Build The Linux Shipping Package

Use this guide on the Linux PC. It creates the single archive needed to publish
Gorilla Protocol through the existing Google Cloud Pixel Streaming host.

## Before You Start

- Ubuntu 22.04 is recommended.
- Use a machine with at least 32 GB RAM and 200 GB free SSD space.
- Install the latest stable Vulkan driver for the machine's NVIDIA or AMD GPU.
- Sign in at Epic's [Unreal Engine for Linux](https://www.unrealengine.com/en-US/linux)
  page, download the precompiled Unreal Engine 5.8 Linux ZIP, and extract it to
  `$HOME/Unreal/UE_5.8`.

If the Engine was extracted elsewhere, change `UE_ROOT` in step 3.

## 1. Install The Build Tools

Open Terminal and run:

```bash
sudo apt update
sudo apt install -y build-essential git git-lfs python3 unzip vulkan-tools
git lfs install
```

## 2. Download Gorilla Protocol

For a new checkout, run:

```bash
mkdir -p "$HOME/Projects"
cd "$HOME/Projects"
git clone https://github.com/jbisaccia-9/gorilla-protocol.git
cd gorilla-protocol
git lfs pull
```

If the repository already exists on the Linux PC, update it instead:

```bash
cd "$HOME/Projects/gorilla-protocol"
git pull --ff-only
git lfs pull
```

## 3. Verify Unreal Engine 5.8

Run:

```bash
export UE_ROOT="$HOME/Unreal/UE_5.8"
test -x "$UE_ROOT/Engine/Binaries/Linux/UnrealEditor"
test -x "$UE_ROOT/Engine/Build/BatchFiles/Linux/Build.sh"
test -x "$UE_ROOT/Engine/Build/BatchFiles/Linux/SetupToolchain.sh"
```

No output means the checks passed. If a check fails, correct `UE_ROOT` before
continuing.

## 4. Build The Upload Archive

From the repository directory, run:

```bash
cd "$HOME/Projects/gorilla-protocol"
export UE_ROOT="$HOME/Unreal/UE_5.8"
./Scripts/build_linux_shipping.sh
```

The command performs the complete build pipeline:

1. Installs Epic's UE5.8 Linux toolchain.
2. Compiles the Gorilla Protocol editor module.
3. Generates the required `L_Boot.umap` startup map.
4. Runs the repository validation checks.
5. Builds, cooks, stages, and packages the Linux Shipping game.
6. Creates the compressed cloud-upload archive and its SHA-256 checksum.

The first build can take a long time while Unreal compiles code and shaders. Keep
the Terminal open until it prints `Linux Shipping archive ready`.

## 5. Return This File

The finished file is:

```text
$HOME/Projects/gorilla-protocol/Artifacts/GorillaProtocol-Linux.tar.gz
```

Return that `.tar.gz` file. Do not return the Unreal Engine ZIP, the entire project
directory, credentials, or Google Cloud configuration files.

The checksum is stored beside it at:

```text
$HOME/Projects/gorilla-protocol/Artifacts/GorillaProtocol-Linux.tar.gz.sha256
```

## Common Errors

- `UnrealEditor not found`: `UE_ROOT` does not point to the extracted UE5.8 folder.
- `git: 'lfs' is not a git command`: rerun the package installation in step 1.
- Vulkan or renderer errors: update the GPU driver and verify `vulkaninfo --summary`.
- Out-of-memory or disk errors: close other applications and confirm free space with
  `df -h "$HOME"` and memory with `free -h`.
- Missing `L_Boot.umap`: rerun `./Scripts/build_linux_shipping.sh`; do not skip the
  editor compilation stage.

Epic's current setup references are the
[Linux development quickstart](https://dev.epicgames.com/documentation/unreal-engine/linux-development-quickstart-for-unreal-engine)
and [Linux development requirements](https://dev.epicgames.com/documentation/unreal-engine/linux-development-requirements-for-unreal-engine).
