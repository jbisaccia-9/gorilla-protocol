# Offline Rebuild Drive

The flash-drive bundle is a clean shallow Git checkout of the active rebuild. It
contains the exact source revision, approved visual target, Git LFS content,
historical Java and WebGL snapshots, validation scripts, asset contract, licensing
manifest, and Linux build instructions. It excludes local cloud configuration,
credentials, generated caches, Unreal Engine, and the retired Shipping binary.

## On The Linux PC

Copy the `gorilla-protocol` directory from the drive to a Linux SSD. Do not build
directly on the flash drive.

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
