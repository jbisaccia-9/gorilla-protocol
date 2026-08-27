# Authored Content Required

The active project contains no procedural presentation fallback. Create or import
production assets in Unreal Editor under `Content/GorillaProtocol/` using the
layout and standards in `Docs/CONTENT_PIPELINE.md`.

The exact minimum package contract is `Build/VerticalSliceAssets.txt`. It starts
with:

- `Core/DA_VerticalSlice`
- `Maps/L_ScimmiaDiMare`
- Production first-person and full-body Bruno meshes and Animation Blueprints
- Production guard mesh, Animation Blueprint, and three archetype Blueprints
- Authored Enhanced Input mapping, CommonUI HUD, guard StateTree, and perception data
- Suppressed P9 and guard SMG meshes and definitions
- Representative yacht material, Niagara, MetaSound, ambience, and Italian voice

Do not add Engine basic shapes, mannequin characters, floating debug labels, or
temporary weapons to make a package pass. The beauty corner is approved visually
before the remainder of the mission is built.
