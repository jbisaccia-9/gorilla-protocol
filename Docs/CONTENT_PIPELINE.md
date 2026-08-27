# Content-First Production

The retired prototype failed because presentation was synthesized in C++. The
rebuild reverses ownership: artists author the map, characters, animation,
materials, lighting, VFX, sound, and UI in Unreal. C++ supplies stable rules and
validation only. A missing asset is a build failure, never permission to draw a
cube, sphere, label, or colored debug light in Shipping.

## Milestones

1. **Beauty corner:** one final-quality 20 x 20 meter yacht area, Bruno's final
   first-person arms, one guard, one weapon, representative rain, impacts, sound,
   and lighting. No level expansion until this frame matches the visual target.
2. **First encounter:** two guards and four real approaches with production
   movement, Gorilla Grip, Knuckle Rush, pistol, detection, and restart.
3. **Complete mission:** authored superyacht route, three enemy archetypes,
   checkpoints, adaptive escape, result scoring, and Italian performance.
4. **Optimization:** packaged 1080p60 validation, Unreal Insights capture,
   automated screenshots, cook validation, then a separate stream soak.

## Content Layout

```text
Content/GorillaProtocol/
  AI/                 StateTrees, perception data, encounter assets
  Animation/          motion database, montages, warping, Control Rigs
  Audio/              MetaSounds, ambience, Foley, consented Italian voice
  Characters/Bruno/   first-person and full-body rigs, materials, anim BPs
  Characters/Guards/  shared skeleton, variants, equipment, anim BP
  Core/               DA_VerticalSlice and content definitions
  Environment/Yacht/  modular kit, props, decals, materials, level instances
  FX/                 Niagara systems and effect-type scalability assets
  Input/              Enhanced Input actions and mapping contexts
  Maps/               authored mission and test maps
  UI/                 CommonUI/UMG widgets
  Weapons/            meshes, animation, definitions, audio, effects
```

## Character And Rendering Rules

- Bruno uses separate production first-person arms/torso and a full world body.
  The world body is hidden only from the primary camera, still casts a shadow,
  and appears in the authored reflection test.
- Prefer stable fur cards, clumps, flow maps, and anisotropic shading over a
  full-body strand groom that shimmers under TSR and video compression.
- Guards share a production skeleton but require readable body, head, equipment,
  and weapon silhouettes plus complete locomotion, cover, reaction, and death sets.
- Use Motion Matching for world locomotion only after the dataset proves starts,
  stops, strafes, and turns. Use Motion Warping for grabs and traversal, Control
  Rig for contact correction, and authored first-person additive weapon motion.
- Environment geometry is modular and Nanite-ready with authored collision,
  nav links, patrol splines, cover markers, physical materials, and sheltered rain.

## Asset And License Rules

- Original hero characters require work-for-hire terms covering source files,
  modification, interactive media, marketing, and derivative use.
- Never use ripped franchise assets, branded likenesses, iconic ape costumes,
  cloned performers, NC/ND assets, or ambiguous uploads.
- Raw marketplace assets live in a private production depot when redistribution
  is restricted. Only cooked output and repository-safe derivatives may ship.
- Every committed model, texture, animation, sound, font, map, and Unreal asset
  has a row in `Licenses/AssetManifest.csv` with proof retained outside Git.
- `Build/VerticalSliceAssets.txt` is the package contract. Packaging fails until
  every listed production asset exists and is licensed.
