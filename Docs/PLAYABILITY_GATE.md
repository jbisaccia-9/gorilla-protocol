# Playability Gate

`Playable` means a complete, repeatable game loop. It does not mean that the
project compiles, the editor opens, a map loads, or the player can move around.

## Gate A: Official Lyra Baseline

The untouched Lyra `L_Expanse` experience must run on the target Linux PC before
Gorilla Protocol code or content is integrated.

- The player can move, aim, fire, reload, use abilities, take damage, die, and respawn.
- Bots navigate, acquire targets, attack, take damage, die, and re-enter play.
- Weapons provide animation, sound, muzzle flash, impacts, hit confirmation, and recoil.
- The match has a visible objective, score, end state, and repeatable restart.
- Mouse input remains responsive during combat with no recurring frame stalls.
- The game completes three consecutive ten-minute sessions without a crash or blocker.
- The target workstation maintains 60 FPS at 1080p on an agreed scalability preset.

Any failure blocks customization. Engine setup and performance are fixed against
Lyra first, where Gorilla-specific code cannot obscure the cause.

## Gate B: Gorilla Conversion

The first Gorilla Protocol milestone is one converted Lyra combat room, not a
complete mission.

- The player has a production-quality gorilla first-person body and full-body shadow.
- Locomotion, aiming, firing, reloading, melee, damage, death, and respawn remain intact.
- Bruno speaks only Italian, with subtitles and no synthetic placeholder barks.
- One guard archetype retains Lyra-quality navigation, attacks, reactions, and death.
- No mannequin, Engine primitive, placeholder weapon, or debug label is visible.
- The conversion passes every Gate A stability and performance requirement.

## Gate C: Mission Slice

Only after Gate B passes do we build the five-to-ten-minute yacht mission: briefing,
infiltration, one stealth/combat encounter, ledger objective, extraction, victory,
defeat, checkpoint, and immediate replay.

No build is described as playable until all behaviors in its active gate have been
demonstrated in a packaged local build.
