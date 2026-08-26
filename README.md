# Gorilla Golden Eye: Operazione Banana

> **Historical iteration:** active development and the live UE5 cloud release
> have moved to [Gorilla Protocol](https://github.com/jbisaccia-9/gorilla-protocol).
> This repository is retained only as the original WebGL history and is also
> preserved under `archive/webgl-classic/` in the unified repository.

A browser-native 3D first-person shooter starring Gori Kongo, an original
Italian-speaking gorilla secret agent. The game carries a classic console-spy
framework into a modern WebGL presentation with desktop and touch controls.

## Features

- True Three.js WebGL environments with procedural materials, lighting, fog, and shadows
- Three missions with distinct maps, palettes, and objectives
- Guards with patrol, suspicion, investigation, burst-fire, stagger, and melee states
- Procedural 3D guards, pickups, world effects, and gorilla weapon viewmodel
- Visible world-space projectiles, headshots, near misses, and directional damage feedback
- Wall collision, health, ammunition, pickups, scoring, and level progression
- Mouse-look, aim-down-sights, sprint, keyboard controls, and a mobile touch interface
- Procedural Web Audio and optional Italian browser speech synthesis
- Adaptive rendering quality and a self-contained HTML release

## Play

**[Play Gorilla Golden Eye in your browser](https://jbisaccia-9.github.io/gorilla-golden-eye/)**

Open `gorilla-golden-eye-standalone.html` directly, or serve the repository:

```bash
python3 -m http.server 8081
```

Then visit `http://localhost:8081`.

## Controls

- `WASD`: move and strafe
- Mouse: look
- Left click or `Space`: fire
- Right click: aim down sights
- `Shift`: sprint
- `R`: reload
- `P` or `Escape`: pause
- `M`: mute
- Touch: left movement pad, right look region, fire and reload buttons

## Mission

Each of the three levels requires Gori Kongo to recover the glowing cyan intelligence pickup, eliminate every guard, and return to the illuminated extraction point. Ammo and health pickups are scattered through the map. Health partially recovers between missions.

## Build

Install the build-time dependency, compile the local Three.js source into an
import-free browser bundle, and rebuild the standalone edition:

```bash
npm install
npm run build:standalone
```

The committed `game.js` and standalone HTML are ready to run without installing
packages. The build dependency is required only when editing `game-3d.source.js`.

## Privacy

The game runs entirely in the browser. It contains no analytics, credentials,
personal data, external API integrations, or runtime network requests.

## License

Released under the [MIT License](LICENSE). Copyright (c) 2026 Joseph Bisaccia.

## Project Layout

- `index.html`, `styles.css`: page structure and interface
- `game-3d.source.js`: maintainable WebGL game source
- `game.js`: generated import-free browser bundle
- `audio.js`: procedural effects and optional Italian speech
- `vendor/`: locally vendored Three.js revision 183 modules
- `build-game.js`: esbuild bundler for the browser runtime
- `build-standalone.js`: repeatable single-file release builder
- `gorilla-golden-eye-standalone.html`: downloadable self-contained edition
