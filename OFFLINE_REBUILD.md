# Offline Rebuild Drive

The flash-drive bundle is a clean shallow Git checkout of the active rebuild. It
contains the exact source revision, approved visual target, Git LFS content,
historical Java and WebGL snapshots, validation scripts, asset contract, licensing
manifest, and Linux build instructions. It excludes local cloud configuration,
credentials, generated caches, Unreal Engine, and the retired Shipping binary.

## On The Linux PC

Open the flash drive in the file manager, open a Terminal in the
`GorillaProtocol-Rebuild` folder, and run the location-independent installer:

```bash
bash gorilla-protocol/Scripts/install_offline_copy.sh
```

The installer copies the complete checkout, including its hidden `.git`
directory and local Git LFS objects, to `$HOME/Projects/gorilla-protocol`. It
refuses to overwrite a non-empty destination. Do not build directly on the flash
drive.

If only the backup archive is available, extract it first and run the same
command from the extracted `GorillaProtocol-Rebuild` folder:

```bash
tar -xzf GorillaProtocol-Rebuild.tar.gz
cd GorillaProtocol-Rebuild
bash gorilla-protocol/Scripts/install_offline_copy.sh
```

```bash
cd "$HOME/Projects/gorilla-protocol"
git status
git lfs install
git lfs checkout
export UE_ROOT="$HOME/Unreal/UE_5.8"
./Scripts/validate_project.sh
./Scripts/build_editor.sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" GorillaProtocol.uproject
```

The approved 3D direction is `Docs/Art/vertical-slice-visual-target.png`, with its
binding requirements in `Docs/VISUAL_TARGET.md`. Author or legally import every
asset listed in `Build/VerticalSliceAssets.txt`, add each license row to
`Licenses/AssetManifest.csv`, and run:

```bash
./Scripts/validate_vertical_slice.sh
```

That command is expected to fail until the production character, animation,
weapon, environment, map, AI, UI, VFX, audio, and Italian voice assets exist. Do
not bypass it with Engine primitives or proxy art.

Once the local packaged game meets `Docs/VERTICAL_SLICE.md`:

```bash
./Scripts/build_linux_shipping.sh
```

Unreal Engine 5.8 and the missing production assets are not included because they
do not currently exist in the project and the Engine is much larger than this
drive. The retired binary is preserved separately only for history and must not
be uploaded or presented as the rebuild.
