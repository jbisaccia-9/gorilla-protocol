# Implementation Roadmap

## Gate 0: Toolchain

- Free at least 150 GB of disk space
- Install Unreal Engine 5.8, compatible full Xcode, and Git LFS
- Run `Scripts/bootstrap_project.sh`
- Compile the Editor target and run source, automation, and data-validation gates

## Gate 1: Production Asset Blockout

- Commission or license the hero gorilla, guard, weapon, and environment kits
- Record every asset in `Licenses/AssetManifest.csv`
- Create Blueprint children for player and guards
- Replace all Engine proxy meshes
- Build the first-person animation blueprint and Control Rig

## Gate 2: Authored Mission

- Replace the runtime graybox with `L_OperationPortoNero`
- Author nav mesh, patrol routes, cover slots, encounter groups, and spawn points
- Move tuning into Data Assets
- Replace the C++ tactical state switch with StateTree plus EQS cover queries
- Add stealth, alarms, optional relay sabotage, scoring, checkpoints, and restart

## Gate 3: Presentation

- Implement master materials, wetness, decals, and level lighting
- Add Niagara and MetaSound production systems
- Record and integrate Italian voice, subtitles, facial animation, and localization
- Replace the fallback HUD with a CommonUI/UMG implementation

## Gate 4: Performance And Release

- Package and profile Mac and Windows builds
- Add controller remapping, accessibility, graphics settings, and save data
- Run 15-minute soak, combat fairness, objective, extraction, and restart tests
- Eliminate shader hitches, texture-pool warnings, VSM overflows, and cook warnings

## Console Gate

Console work starts only after platform-holder approval, devkits, restricted SDKs,
secure CI, and authorized Unreal source access are in place. Console credentials,
SDK content, and NDA material must never enter this public repository.
