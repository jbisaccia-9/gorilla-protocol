# Approved Visual Target

**Status:** Approved by the project owner on 2026-08-27

![Approved Gorilla Protocol visual target](Art/vertical-slice-visual-target.png)

This frame is the binding art direction for the beauty corner and the complete
vertical slice. It is generated concept art, not an in-engine screenshot. The
first Unreal approval capture must reproduce its visual language without tracing
the image or substituting flat proxy geometry.

## Frame Contract

- **Camera:** grounded first-person view, 16:9, approximately 90-degree horizontal
  field of view. Bruno's forearms, hands, and weapon occupy the lower-right third
  without blocking the encounter.
- **Bruno:** anatomically credible broad gorilla forearms and hands, wet black fur
  with readable clumps and direction, correct weapon contact, no human sleeves,
  floating hands, cubes, spheres, or cartoon proportions.
- **Weapon:** original compact suppressed P9 visual design with worn blued metal,
  believable controls and attachments, stable sight picture, and one small banana
  charm. The charm is the only saturated yellow foreground accent.
- **Environment:** authored Mediterranean cliffside intelligence facility with
  brutalist concrete, stone, steel, glass, cypress, communications equipment,
  ocean exposure, layered elevation, and multiple readable tactical routes.
- **Surfaces:** wetness is material-driven. Puddles, rough concrete, worn metal,
  glass, foliage, and painted fixtures retain distinct roughness and normal detail.
- **Lighting:** storm-blue ambient and moon key, warm tungsten interiors, and
  controlled red alarm practicals. Reflections connect the palette without turning
  the scene into neon cyberpunk.
- **Guards:** original dark security uniforms with readable silhouette, equipment,
  pose, muzzle direction, and cover usage. No outlines, floating labels, colored
  state lights, or glowing path markers.
- **Combat effects:** compact muzzle flash, surface-aware impact, smoke, rain,
  mist, and restrained tracer readability. Effects cannot hide the target or rely
  on subpixel sparkle that collapses under video compression.
- **Tone:** credible espionage first, dry physical comedy second. No parody costume,
  franchise likeness, banana wallpaper, or constant joke signage.

## Beauty-Corner Shot

The first 20 x 20 meter production scene must contain all of the following in one
playable camera path:

1. Bruno's final first-person arms and P9 with idle, aim, fire, reload, and impact response.
2. One production guard with patrol, investigate, cover, fire, stagger, and death animation.
3. Wet exterior cover, an illuminated interior, ocean/cliff vista, cypress, and communications prop.
4. Storm ambience, sheltered rain behavior, weapon report and tail, Foley, guard cue, and one consented Italian Bruno line.
5. Final moon, skylight, tungsten, alarm, fog, reflection, impact, and rain treatment.
6. A full-body Bruno shadow and one authored reflection proving the world-body setup.

## Rejection Conditions

Reject the shot if any of these are visible:

- Engine primitive, mannequin, temporary weapon, checker material, floating debug text, or AI state light
- Flat-color material standing in for PBR surface work
- Fur shimmer, disconnected hand contact, foot sliding, sight misalignment, or muzzle origin mismatch
- Uniformly glossy rain, indoor rain, red light washing out silhouettes, or unreadable black crush
- Generic sci-fi architecture, cartoon ape styling, branded weapon, or recognizable franchise design
- A static beauty render that cannot be played at target frame time

## Runtime Approval

- Capture at 1920 x 1080 from a local packaged build, not Pixel Streaming.
- Sustain 60 FPS with GPU p95 at or below 14 ms and Game/Render threads at or below 6 ms.
- Use TSR from a profiled 67-75 percent internal resolution only if fur, rain, and
  weapon edges remain stable in motion.
- Record a 60-second play path covering idle, movement, aim, fire, guard response,
  alarm transition, interior/exterior movement, shadow, and reflection.
- Review the capture side by side with this target. Approval is visual and
  technical; passing only one dimension is insufficient.
