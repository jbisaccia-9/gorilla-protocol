# Gorilla Protocol

`Gorilla Protocol` is being rebuilt from zero as an original Unreal Engine 5.8
single-player spy FPS. Agent Bruno is a physically powerful gorilla operative who
speaks only Italian. The target is a funny, replayable ten-minute mission with
credible current-generation presentation, not a franchise imitation.

![Operazione Scimmia di Mare visual target](Docs/Art/vertical-slice-visual-target.png)

This direction was approved on 2026-08-27. The binding shot breakdown and
rejection criteria are in [`Docs/VISUAL_TARGET.md`](Docs/VISUAL_TARGET.md).

## Rebuild Status

The procedural prototype was rejected and retired at Git tag
`graybox-retired-2026-08-26`. Its public GPU VM is stopped and there is currently
no playable release. The active source intentionally does not contain a fallback
level, proxy gorilla, proxy guards, placeholder weapon, or canvas HUD.

Current active work provides:

- The authored mission and quality contract in [`Docs/VERTICAL_SLICE.md`](Docs/VERTICAL_SLICE.md)
- The approved frame contract in [`Docs/VISUAL_TARGET.md`](Docs/VISUAL_TARGET.md)
- The 3D, animation, audio, and licensing workflow in
  [`Docs/CONTENT_PIPELINE.md`](Docs/CONTENT_PIPELINE.md)
- A minimal C++ experience bootstrap that refuses to create substitute art
- A package gate requiring every production asset in
  [`Build/VerticalSliceAssets.txt`](Build/VerticalSliceAssets.txt)
- UE5.8 rendering, source validation, Git LFS, and dormant Pixel Streaming infrastructure

The first deliverable is one final-quality 20 x 20 meter superyacht beauty corner
with Bruno's production first-person body, one animated guard, the suppressed P9,
wet materials, rain, impacts, lighting, and representative sound. Map expansion
does not begin until that scene matches the visual target.

## Vertical Slice

**Operazione Scimmia di Mare** is a ten-minute mission aboard a storm-lashed
superyacht off the Amalfi coast. Bruno boards by anchor chain, steals an encrypted
ledger, and escapes on an undersized tender.

Signature mechanics:

- **Gorilla Grip:** strike, grab, carry, and throw guards or authored props
- **Knuckle Rush:** a loud quadrupedal charge through guards and marked barriers
- **Primate Traversal:** tactical mantles, pipes, hangs, and exterior ledges
- **Bruno Button:** an Italian contextual quip or a deliberate vocal lure
- **Banana Decoy:** a limited authored distraction and slip interaction

The first encounter must support ghosting, distraction, grabbing, and shooting as
real systems. Detection changes the mission instead of failing it. Guards use
local knowledge, interruptible radio calls, visible reinforcement entrances, and
readable attack animation. Full criteria are in
[`Docs/VERTICAL_SLICE.md`](Docs/VERTICAL_SLICE.md).

## Production Boundary

C++ owns deterministic rules, content contracts, validation, and stable runtime
interfaces. Unreal assets own characters, weapons, input, animation, AI StateTrees,
encounters, mission placement, lighting, materials, Niagara, MetaSounds, voice,
and CommonUI. Missing presentation is a failed build, never permission to spawn
Engine primitives or debug labels.

Every imported asset must be legally redistributable for this repository and
recorded in [`Licenses/AssetManifest.csv`](Licenses/AssetManifest.csv). Restricted
raw marketplace content belongs in a private production depot, not public Git.

## Open For Production

Prerequisites:

- Unreal Engine 5.8
- Linux or Windows workstation with a supported SM6 GPU
- Git LFS
- At least 32 GB RAM and 200 GB free SSD space

On Linux:

```bash
git lfs install
git lfs pull
export UE_ROOT="$HOME/Unreal/UE_5.8"
./Scripts/build_editor.sh
"$UE_ROOT/Engine/Binaries/Linux/UnrealEditor" GorillaProtocol.uproject
```

Create the authored assets at the exact paths in
[`Build/VerticalSliceAssets.txt`](Build/VerticalSliceAssets.txt). The project is
expected to report a missing startup map until `L_ScimmiaDiMare.umap` is authored
and committed through Git LFS.

Source-only validation:

```bash
./Scripts/validate_project.sh
```

Production-content gate, expected to fail until the vertical slice exists:

```bash
./Scripts/validate_vertical_slice.sh
```

Linux Shipping instructions are in
[`LINUX_BUILD_README.md`](LINUX_BUILD_README.md). Packaging and cloud deployment
remain blocked until the content gate passes and a local packaged playtest meets
the acceptance bar. Pixel Streaming is a delivery mechanism, not evidence of game
quality.

## Historical Iterations

- `graybox-retired-2026-08-26` preserves the rejected UE procedural prototype.
- [`archive/java-swing/`](archive/java-swing/) preserves the original Java version.
- [`archive/webgl-classic/`](archive/webgl-classic/) preserves the earlier WebGL version.

Git history is the archive; the repository root contains only the active rebuild.

This project is not affiliated with Nintendo, Rare, GoldenEye, James Bond, MGM,
EON Productions, Sony, or Microsoft. Do not use ripped franchise assets, branded
likenesses, cloned performers, or NDA-restricted console material.
