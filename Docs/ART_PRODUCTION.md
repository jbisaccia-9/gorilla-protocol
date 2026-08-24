# Art And Rendering Production Standard

## Rendering Baseline

- Unreal Engine 5.8 deferred renderer and Shader Model 6
- Lumen software ray tracing as the shared baseline
- Virtual Shadow Maps and TSR
- Nanite for validated rigid architecture and props
- Conventional skeletal meshes with authored LODs for characters
- One moon directional light, skylight, volumetric fog, and restrained practical lights
- ACES color pipeline, narrow exposure range, subtle bloom, and restrained motion blur

Mac is an editor and gameplay-development target, not a console-performance
reference. Hardware Lumen and MegaLights are not available on Mac. Validate
Windows and console profiles on their actual target hardware.

## Hero Gorilla

- Full-body production sculpt and game mesh
- Separate first-person arms with matching materials and proportions
- Groom or fur-card solution with scalable density
- Face rig supporting Italian phonemes and performance capture cleanup
- Control Rig, FBIK contacts, weapon IK, foot placement, and aim offsets
- 4K albedo, normal, packed ORM, micro-normal, and groom textures
- LODs, physics asset, hit zones, facial LOD strategy, and platform budgets

Required animations include idle, walk, run, sprint, crouch, starts/stops,
turns, vault, lean, recoil, reload, equip, melee, hit reactions, death, and
first-person additive layers.

## Environment

- Approximately 200 x 200 meters
- Modular exterior and interior kits with physically thick Lumen geometry
- Nanite-ready concrete, rock, pipe, debris, dock, and industrial prop meshes
- Trim-sheet architecture plus hero props and decals
- Material Parameter Collection for global wetness and rain response
- PBR masters for hard surface, organic, decal, glass, skin, eyes, and fur
- Packed ORM textures, reusable detail normals, macro variation, and vertex grime

## Effects And Audio

Niagara systems must cover muzzle flash, tracers, physical-surface impacts,
sparks, smoke, rain interaction, alarm strobes, shell ejection, and extraction.
Every system requires pooling, significance, culling, and an Effect Type.

MetaSounds and authored recordings must cover weapons, Foley, footsteps by
surface, guard cues, weather, interiors, impacts, UI, and Italian dialogue.
Do not use browser speech synthesis in the production build.

## Budgets

Performance mode targets 60 fps with a 15 ms GPU budget. Quality mode targets
30 fps with a 30 ms GPU budget. The slice should remain below 12 active guards,
8 full-rate skeletal evaluations, 8 visible shadowed local lights, and 1 ms of
Niagara GPU work in performance mode.

Profile packaged builds with Unreal Insights, GPU Visualizer, Lumen views,
Nanite views, shader complexity, and Virtual Shadow Map cache visualization.
