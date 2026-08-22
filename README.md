# Gorilla Golden Eye: Operazione Banana

A dependency-free browser first-person shooter starring Gori Kongo, an original
Italian-speaking gorilla secret agent. The game combines a retro raycast look
with modern desktop and touch controls.

## Features

- Three missions with distinct maps and objectives
- Guards that pursue, fire visible projectiles, and attack at close range
- Wall collision, health, ammunition, pickups, scoring, and level progression
- Mouse-look, keyboard controls, and a mobile touch interface
- Procedural Web Audio and optional Italian browser speech synthesis
- A self-contained HTML release with all code and art embedded

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
- `R`: reload
- `P` or `Escape`: pause
- `M`: mute
- Touch: left movement pad, right look region, fire and reload buttons

## Mission

Each of the three levels requires Gori Kongo to recover the glowing cyan intelligence pickup and eliminate every guard. Ammo and health pickups are scattered through the map. Health partially recovers between missions.

## Build

The source edition is split into `index.html`, `styles.css`, `audio.js`, `game.js`, and two generated PNG sprites. Rebuild the self-contained edition after any source change:

```bash
node build-standalone.js
```

## Privacy

The game runs entirely in the browser. It contains no analytics, credentials,
personal data, external API integrations, or runtime network requests.

## Project Layout

- `index.html`, `styles.css`: page structure and interface
- `game.js`: rendering, controls, missions, enemy AI, and combat
- `audio.js`: procedural effects and optional Italian speech
- `assets/`: bundled generated character art
- `build-standalone.js`: repeatable single-file release builder
- `gorilla-golden-eye-standalone.html`: downloadable self-contained edition
