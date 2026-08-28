# Gorilla Protocol

`Gorilla Protocol` is an original Unreal Engine 5.8 first-person spy-combat
prototype. Bruno is a gorilla operative, speaks only Italian, carries a suppressed
sidearm, and can use a sprinting knuckle rush or a spoken lure against guards.

![Approved visual direction](Docs/Art/vertical-slice-visual-target.png)

The image above is concept art and the visual target, not an in-engine screenshot.
The current build is a **playable combat slice**, not a PS5-quality finished game.

## Build And Play On Linux

Use the Linux PC that already has Unreal Engine 5.8.1. From this repository run
one command:

```bash
./Scripts/build_and_play_linux.sh
```

That command validates the source, compiles the Editor module, imports the bundled
CC0 models/textures/audio, authors and saves the mission map, validates the
generated assets, and launches the game directly. It does not require Java,
Blender, Fab, an API key, or cloud access.

Controls:

- `WASD`: move
- `Mouse`: look
- `Left Shift`: sprint
- `Left Mouse`: fire
- `R`: reload
- `F` or `Right Mouse`: punch; sprint first for Knuckle Rush
- `E`: take the encrypted ledger
- `Q`: speak Italian and lure unaware guards
- `Space`: jump

Mission: cross the storm-lit facility terrace, take the encrypted ledger from the
red console, then return to the green extraction marker. Guards patrol, investigate
Bruno's voice, pursue on sight, shoot, take damage, and can be knocked down.

## Shipping Package

After the local build has been played successfully:

```bash
./Scripts/build_linux_shipping.sh
```

The package is written to `Artifacts/GorillaProtocol-Linux.tar.gz`. Do not deploy
that archive until the local frame rate and gameplay have been accepted.

## Project Structure

- `Source/GorillaProtocol`: player, combat, guard AI, mission, and HUD C++
- `Scripts/Unreal/bootstrap_playable.py`: deterministic asset import and map authoring
- `RawContent`: redistributable CC0 source assets and project-generated Italian audio
- `Licenses`: complete source and derived-asset provenance
- `Docs`: approved direction and longer-term gameplay contract
- `archive`: historical Java and WebGL iterations retained in this single repository

Source validation is available separately with `./Scripts/validate_project.sh`.
The generated-content gate is `./Scripts/validate_vertical_slice.sh`.

The project has no runtime network calls, secrets, API credentials, personal data,
or paid marketplace content. It is not affiliated with Nintendo, Rare, GoldenEye,
James Bond, MGM, EON Productions, Sony, or Microsoft.
