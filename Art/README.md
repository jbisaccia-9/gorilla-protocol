# Coastal Encounter

This is the Blender art workspace for the approved coastal-facility direction.
It can be inspected on the Mac without Unreal, Lyra, or the Linux PC.

Open `OPEN_COASTAL_SCENE.command` from the repository root to inspect the generated
scene. `BUILD_COASTAL_SCENE.command` rebuilds and renders it from source.

## Scene

- A lower cliffside terrace, central stair, upper operations room and sheltered archive approach.
- Stone joints, coping, wet paving and puddles, window mullions, furnished interior,
  roof railings, parabolic dish, radio mast, cypress and coastal vegetation.
- A modeled gorilla viewmodel with individual gripping fingers, coarse hair,
  suppressed carbine, machined controls, optic, and banana charm.
- Two original guards with detailed equipment and transform-based presentation animation.
- Storm-blue environment, warm practical lights, red alarm lamps, sea and mist.

## Inspect In Blender

The saved camera is `PLAYER | Lower terrace`. Numpad 0 toggles the camera view.
The Scene collection also contains an overview camera and a stair-approach camera.
Use Blender's View > Navigation > Walk Navigation to inspect the geometry.
This navigates the art scene; it does not enable game logic or collisions.

## What Is Verified

`scene-checks.json`, when present, reports actual asset-presence checks and the
evaluated triangle count from the generated scene. PNGs are renders of that scene.
Neither a successful render nor this report is a gameplay/performance approval.

The first render has known art gaps: foliage silhouettes are too chunky, forearm
hair is sparse, and the lighting has less depth and contrast than the approved
reference. These must be improved before treating the scene as final-quality art.

## Remaining Game Work

Lyra has not been downloaded. Unreal has not been run on this Mac. The game still
needs skinned character rigs, locomotion and combat animation, baked engine
materials, collision/navigation, Lyra integration, Italian voice recordings,
the mission loop, and measured gameplay on the Linux PC. The procedural materials
are stored in the .blend and require baking or recreation for engine export.

Keep this scene as reusable source art. Do not substitute the renders for runtime
evidence or publish a Play button for this art build.
