# Gorilla Protocol: Lyra Reset

Gorilla Protocol is being rebuilt as an Unreal Engine 5.8 tactical-comedy FPS on
top of Epic's official Lyra Starter Game. The previous generated graybox is
archived at Git tag `pre-lyra-prototype-2026-08-28`; it is not the foundation for
the new game and is not presented as playable.

## Current Milestone

**Installing on Ubuntu? Follow [the Linux setup README](README_LINUX_SETUP.md).**
It separates native Linux downloads from Mac-only Launcher/Lyra acquisition,
includes transfer without a flash drive, and provides validation/launch commands.

**Continuing on Linux or another device? Start with the
[Linux handoff](Docs/LINUX_HANDOFF.md).** It records the current branch, asset
recovery, remaining work, two-agent responsibilities, and usage-saving rules.
Project files are recoverable from Git/LFS; chat history is not required.

The Blender coastal encounter is now being built on the Mac. Open
`OPEN_COASTAL_SCENE.command` to inspect the generated `.blend`, or
`BUILD_COASTAL_SCENE.command` to regenerate it. See [the art workspace](Art/README.md)
for what is modeled and what remains before gameplay. This does not require Lyra.

![Blender art progress, not a gameplay screenshot](Art/CoastalSlice/encounter-refined.png)

## Gameplay Baseline

Run the untouched Lyra Expanse shooter experience on the target Linux workstation.
This establishes a proven baseline for locomotion, weapons, bots, damage, death,
respawning, UI, audio, effects, match flow, stability, and performance before any
Gorilla Protocol customization.

Lyra is free, but its source and content must be acquired under the developer's
Epic account and are not copied into this public repository.

Once the complete `LyraStarterGame` folder is present on the Linux PC, run:

```bash
./Scripts/Lyra/build_and_play_baseline_linux.sh
```

The script automatically locates Unreal Engine and Lyra, verifies the ShooterCore
and ShooterMaps installation, builds `LyraEditor`, and launches `L_Expanse` with
frame and timing counters. Set `UE_ROOT` or `LYRA_ROOT` only if automatic discovery
cannot find them.

## Acceptance Standard

[The playability gate](Docs/PLAYABILITY_GATE.md) is binding for gameplay acceptance.
Blender art production can proceed independently on the Mac. Integration and a
claim of playable gameplay still require the official Lyra baseline to complete
three stable ten-minute sessions at the agreed performance target.

The repository contains no Lyra assets, paid marketplace content, secrets, API
credentials, or personal data. Gorilla Protocol is an original project and is not
affiliated with Nintendo, Rare, GoldenEye, James Bond, MGM, EON Productions, Sony,
Microsoft, or Epic Games.
