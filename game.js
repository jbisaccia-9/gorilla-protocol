(() => {
  "use strict";

  const canvas = document.getElementById("game");
  const context = canvas.getContext("2d", { alpha: false });
  const surface = document.createElement("canvas");
  const view = surface.getContext("2d", { alpha: false });
  const WIDTH = 480;
  const HEIGHT = 270;
  const FOV = Math.PI / 3;
  const FIXED_STEP = 1 / 60;
  const MAX_STEPS = 5;
  const PLAYER_RADIUS = 0.22;
  const MAX_AMMO = 12;
  const REDUCED_MOTION = window.matchMedia("(prefers-reduced-motion: reduce)");
  const TEST_MODE = new URLSearchParams(location.search).has("e2e");

  surface.width = WIDTH;
  surface.height = HEIGHT;
  view.imageSmoothingEnabled = false;
  context.imageSmoothingEnabled = false;

  const ui = {
    levelName: document.getElementById("levelName"),
    objective: document.getElementById("objective"),
    health: document.getElementById("health"),
    healthBar: document.getElementById("healthBar"),
    ammo: document.getElementById("ammo"),
    guards: document.getElementById("guards"),
    intel: document.getElementById("intel"),
    overlay: document.getElementById("overlay"),
    overlayKicker: document.getElementById("overlayKicker"),
    overlayTitle: document.getElementById("overlayTitle"),
    overlayText: document.getElementById("overlayText"),
    startButton: document.getElementById("startButton"),
    pauseOverlay: document.getElementById("pauseOverlay"),
    resumeButton: document.getElementById("resumeButton"),
    restartButton: document.getElementById("restartButton"),
    pauseButton: document.getElementById("pauseButton"),
    muteButton: document.getElementById("muteButton"),
    dialogue: document.getElementById("dialogue"),
    dialogueText: document.getElementById("dialogueText"),
    crosshair: document.getElementById("crosshair"),
    hitMarker: document.getElementById("hitMarker"),
    damageFlash: document.getElementById("damageFlash"),
    statusRegion: document.getElementById("statusRegion"),
    movePad: document.getElementById("movePad"),
    moveStick: document.getElementById("moveStick"),
    lookZone: document.getElementById("lookZone"),
    touchFire: document.getElementById("touchFire"),
    touchReload: document.getElementById("touchReload")
  };

  const audio = window.GorillaAudio || {
    init() {}, setMuted() {}, isMuted() { return true; }, setPaused() {}, event() {}, speakItalian() {}, stopSpeech() {}
  };

  const LEVELS = [
    {
      name: "PORTO NEBBIA",
      subtitle: "Molo doganale · Genova · 02:10",
      objective: "Recupera il manifesto cifrato e neutralizza le guardie.",
      theme: { sky: "#17324a", sky2: "#0d1e29", floor: "#26332f", floor2: "#151e1b", wall: [54, 105, 91], accent: [191, 142, 53] },
      spawn: [1.5, 1.5, 0],
      guards: 6,
      map: [
        "1111111111111111", "1000001000000001", "1011001022201101", "1010000000200101",
        "1010111110200101", "1000100010000101", "1110101011110101", "1000001000010001",
        "1022201111011101", "1000200001000101", "1010201101010101", "1010000100010101",
        "1011110111010101", "1000010001000001", "1011000100011001", "1111111111111111"
      ]
    },
    {
      name: "VILLA VULCANO",
      subtitle: "Isola riservata · Sicilia · 23:42",
      objective: "Sottrai il microfilm dalla sala comunicazioni.",
      theme: { sky: "#4a2720", sky2: "#1c1715", floor: "#3a322b", floor2: "#1b1815", wall: [132, 122, 95], accent: [187, 68, 49] },
      spawn: [1.5, 1.5, 0],
      guards: 8,
      map: [
        "1111111111111111", "1000000010000001", "1011101010111101", "1000101000100001",
        "1110101110101101", "1000100010001001", "1011111011101011", "1000001000100001",
        "1011101110111101", "1010000010000101", "1010111011110101", "1000100000100001",
        "1110101110111101", "1000100010000001", "1010001100111001", "1111111111111111"
      ]
    },
    {
      name: "STAZIONE ORSO",
      subtitle: "Alpi italiane · Settore 7 · 04:05",
      objective: "Recupera il prototipo e chiudi l'operazione.",
      theme: { sky: "#213953", sky2: "#101b27", floor: "#27313a", floor2: "#141a20", wall: [78, 103, 122], accent: [203, 58, 50] },
      spawn: [1.5, 1.5, 0],
      guards: 10,
      map: [
        "1111111111111111", "1000100000100001", "1010101110101101", "1010000010001001",
        "1011111011101011", "1000001000101001", "1111101110101101", "1000100010000101",
        "1010111011110101", "1010001000010001", "1011101111011101", "1000100001000101",
        "1110111101110101", "1000001000000001", "1011100011111101", "1111111111111111"
      ]
    }
  ];

  const dialoguePool = [
    "Andiamo. La notte è giovane e io ho fame.",
    "Questa faccenda puzza. E non sono stato io.",
    "Nessuno tocca le banane dell'agente Kongo.",
    "Eleganza, precisione, pelo impeccabile.",
    "Parlo piano, ma le mie mani fanno rumore.",
    "Un gorilla, una missione, dodici colpi. Basta e avanza."
  ];

  const state = {
    mode: "ready",
    levelIndex: 0,
    map: [],
    player: null,
    guards: [],
    pickups: [],
    particles: [],
    enemyShots: [],
    intel: false,
    score: 0,
    dialogueTimer: 4,
    randomDialogueTimer: 8,
    hitTimer: 0,
    damageTimer: 0,
    muzzleTimer: 0,
    recoil: 0,
    elapsed: 0,
    transitionTimer: 0
  };

  const input = {
    keys: new Set(),
    fire: false,
    moveX: 0,
    moveY: 0,
    lookPointer: null,
    lookX: 0,
    lookY: 0,
    movePointer: null
  };

  let accumulator = 0;
  let lastTime = performance.now();
  let suppressPointerPause = false;
  let hudSnapshot = "";
  const depthBuffer = new Float32Array(WIDTH);

  function seededRandom(seed) {
    let value = seed >>> 0;
    return () => {
      value = (value * 1664525 + 1013904223) >>> 0;
      return value / 4294967296;
    };
  }

  function normalizeAngle(angle) {
    while (angle < -Math.PI) angle += Math.PI * 2;
    while (angle > Math.PI) angle -= Math.PI * 2;
    return angle;
  }

  function tileAt(x, y) {
    const mx = Math.floor(x);
    const my = Math.floor(y);
    if (my < 0 || my >= state.map.length || mx < 0 || mx >= state.map[0].length) return 1;
    return Number(state.map[my][mx]);
  }

  function isOpen(x, y, radius = 0) {
    return tileAt(x - radius, y - radius) === 0 && tileAt(x + radius, y - radius) === 0 &&
      tileAt(x - radius, y + radius) === 0 && tileAt(x + radius, y + radius) === 0;
  }

  function openCells() {
    const cells = [];
    for (let y = 1; y < state.map.length - 1; y += 1) {
      for (let x = 1; x < state.map[y].length - 1; x += 1) {
        if (Number(state.map[y][x]) === 0) cells.push({ x: x + 0.5, y: y + 0.5 });
      }
    }
    return cells;
  }

  function loadLevel(index, preserveVitals = false) {
    const level = LEVELS[index];
    state.levelIndex = index;
    state.map = level.map.slice();
    const previousHealth = state.player ? state.player.health : 100;
    const previousAmmo = state.player ? state.player.ammo : MAX_AMMO;
    state.player = {
      x: level.spawn[0], y: level.spawn[1], angle: level.spawn[2], pitch: 0,
      health: preserveVitals ? Math.min(100, previousHealth + 20) : 100,
      ammo: preserveVitals ? Math.max(6, previousAmmo) : MAX_AMMO,
      cooldown: 0, reload: 0, bob: 0, moving: false
    };
    state.guards = [];
    state.pickups = [];
    state.particles = [];
    state.enemyShots = [];
    state.intel = false;
    state.hitTimer = 0;
    state.damageTimer = 0;
    state.muzzleTimer = 0;
    state.recoil = 0;
    state.transitionTimer = 0;

    const cells = openCells();
    const random = seededRandom(8128 + index * 997);
    const available = cells.filter((cell) => Math.hypot(cell.x - state.player.x, cell.y - state.player.y) > 4);
    for (let i = 0; i < level.guards && available.length; i += 1) {
      const choice = Math.floor(random() * available.length);
      const cell = available.splice(choice, 1)[0];
      state.guards.push({
        x: cell.x, y: cell.y, health: index === 2 && i === 0 ? 3 : 2,
        attack: 0.5 + random(), alert: 0, hit: 0, muzzle: 0, dead: false,
        sway: random() * Math.PI * 2
      });
    }

    const farthest = cells.slice().sort((a, b) =>
      Math.hypot(b.x - state.player.x, b.y - state.player.y) - Math.hypot(a.x - state.player.x, a.y - state.player.y)
    );
    state.pickups.push({ ...farthest[0], type: "intel", bob: 0 });
    const supplyCells = farthest.filter((cell, i) => i > 5 && i % 7 === 0).slice(0, 3);
    if (supplyCells[0]) state.pickups.push({ ...supplyCells[0], type: "ammo", bob: 1.4 });
    if (supplyCells[1]) state.pickups.push({ ...supplyCells[1], type: "health", bob: 2.6 });
    if (supplyCells[2]) state.pickups.push({ ...supplyCells[2], type: "ammo", bob: 3.8 });

    ui.levelName.textContent = level.name;
    ui.objective.textContent = level.objective;
    say(index === 0 ? "Agente Kongo al rapporto. Parlo solo italiano." : "Nuova missione. Stesso gorilla. Andiamo.", true);
    updateHud(true);
  }

  function clearInputs() {
    input.keys.clear();
    input.fire = false;
    input.moveX = 0;
    input.moveY = 0;
    input.movePointer = null;
    input.lookPointer = null;
    ui.moveStick.style.transform = "translate(-50%, -50%)";
  }

  function startGame() {
    audio.init();
    audio.setPaused(false);
    if (state.mode === "victory" || state.mode === "gameOver") {
      state.score = 0;
      loadLevel(0, false);
    }
    state.mode = "playing";
    hideMainOverlay();
    updateHud(true);
    announce("Missione iniziata.");
    requestPointerLock();
  }

  function restartGame() {
    clearInputs();
    state.score = 0;
    loadLevel(0, false);
    state.mode = "playing";
    hidePauseOverlay();
    hideMainOverlay();
    audio.setPaused(false);
    updateHud(true);
    audio.event("button");
    requestPointerLock();
  }

  function pauseGame(fromPointer = false) {
    if (state.mode !== "playing") return;
    state.mode = "paused";
    clearInputs();
    audio.setPaused(true);
    updateHud(true);
    showPauseOverlay();
    if (!fromPointer && document.pointerLockElement === canvas) {
      suppressPointerPause = true;
      document.exitPointerLock();
    }
    announce("Missione in pausa.");
  }

  function resumeGame() {
    if (state.mode !== "paused") return;
    audio.init();
    audio.setPaused(false);
    state.mode = "playing";
    hidePauseOverlay();
    lastTime = performance.now();
    accumulator = 0;
    updateHud(true);
    requestPointerLock();
    announce("Missione ripresa.");
  }

  function requestPointerLock() {
    if (matchMedia("(pointer: fine)").matches && canvas.requestPointerLock && document.pointerLockElement !== canvas) {
      canvas.requestPointerLock().catch(() => {});
    }
  }

  function showPauseOverlay() {
    ui.pauseOverlay.hidden = false;
    ui.pauseOverlay.inert = false;
    ui.pauseOverlay.setAttribute("aria-hidden", "false");
    requestAnimationFrame(() => ui.pauseOverlay.classList.add("is-visible"));
    setTimeout(() => ui.resumeButton.focus(), 30);
  }

  function hidePauseOverlay() {
    ui.pauseOverlay.classList.remove("is-visible");
    ui.pauseOverlay.hidden = true;
    ui.pauseOverlay.inert = true;
    ui.pauseOverlay.setAttribute("aria-hidden", "true");
  }

  function showMainOverlay(kicker, title, text, button) {
    ui.overlayKicker.textContent = kicker;
    ui.overlayTitle.textContent = title;
    ui.overlayText.textContent = text;
    ui.startButton.textContent = button;
    ui.overlay.classList.add("is-visible");
    setTimeout(() => ui.startButton.focus(), 30);
  }

  function hideMainOverlay() {
    ui.overlay.classList.remove("is-visible");
  }

  function say(text, speak = false) {
    ui.dialogueText.textContent = `“${text}”`;
    ui.dialogue.classList.remove("is-hidden");
    state.dialogueTimer = 4.2;
    if (speak) audio.speakItalian(text);
  }

  function announce(text) {
    ui.statusRegion.textContent = text;
  }

  function movePlayer(dx, dy) {
    const player = state.player;
    const nextX = player.x + dx;
    const nextY = player.y + dy;
    if (isOpen(nextX, player.y, PLAYER_RADIUS)) player.x = nextX;
    if (isOpen(player.x, nextY, PLAYER_RADIUS)) player.y = nextY;
  }

  function lineOfSight(x1, y1, x2, y2) {
    const distance = Math.hypot(x2 - x1, y2 - y1);
    const steps = Math.max(1, Math.ceil(distance * 12));
    for (let i = 1; i < steps; i += 1) {
      const t = i / steps;
      if (tileAt(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t) !== 0) return false;
    }
    return true;
  }

  function fireWeapon() {
    const player = state.player;
    if (state.mode !== "playing" || player.cooldown > 0 || player.reload > 0) return;
    player.cooldown = 0.22;
    state.recoil = 1;
    state.muzzleTimer = 0.06;
    ui.crosshair.classList.add("recoil");
    setTimeout(() => ui.crosshair.classList.remove("recoil"), 100);

    if (player.ammo <= 0) {
      audio.event("empty");
      say("Mi serve un caricatore. Subito.", true);
      return;
    }

    player.ammo -= 1;
    audio.event("shot");
    let target = null;
    let targetDistance = Infinity;
    for (const guard of state.guards) {
      if (guard.dead) continue;
      const dx = guard.x - player.x;
      const dy = guard.y - player.y;
      const distance = Math.hypot(dx, dy);
      const difference = Math.abs(normalizeAngle(Math.atan2(dy, dx) - player.angle));
      const aimTolerance = Math.min(0.13, 0.055 + 0.12 / Math.max(1, distance));
      if (difference < aimTolerance && distance < targetDistance && distance < 10 && lineOfSight(player.x, player.y, guard.x, guard.y)) {
        target = guard;
        targetDistance = distance;
      }
    }

    if (!target) {
      spawnParticles(player.x + Math.cos(player.angle) * 2, player.y + Math.sin(player.angle) * 2, "spark", 3);
      return;
    }

    target.health -= 1;
    target.hit = 0.12;
    target.alert = 4;
    state.hitTimer = 0.12;
    ui.hitMarker.classList.add("visible");
    setTimeout(() => ui.hitMarker.classList.remove("visible"), 110);
    audio.event("hit");
    if (target.health <= 0) {
      target.dead = true;
      state.score += 100;
      audio.event("enemyDown");
      say("Uno di meno. Che mira bestiale!", false);
    }
  }

  function reloadWeapon() {
    const player = state.player;
    if (state.mode !== "playing" || player.reload > 0 || player.ammo === MAX_AMMO) return;
    player.reload = 0.82;
    audio.event("reload");
    say("Ricarico. Copritemi... anche se sono solo.", false);
  }

  function hurtPlayer(amount) {
    if (state.mode !== "playing") return;
    state.player.health = Math.max(0, state.player.health - amount);
    state.damageTimer = 0.2;
    ui.damageFlash.classList.add("visible");
    setTimeout(() => ui.damageFlash.classList.remove("visible"), 150);
    audio.event("hurt");
    if (state.player.health <= 0) {
      state.mode = "gameOver";
      audio.event("gameOver");
      audio.stopSpeech();
      showMainOverlay("MISSIONE FALLITA", "Agente a terra", "Mi arrendo... ma solo fino al prossimo tentativo.", "RIPROVA");
      say("Mi arrendo... ma solo per adesso.", true);
      releasePointer();
      updateHud(true);
    }
  }

  function releasePointer() {
    if (document.pointerLockElement === canvas) {
      suppressPointerPause = true;
      document.exitPointerLock();
    }
  }

  function spawnParticles(x, y, type, count) {
    if (REDUCED_MOTION.matches) return;
    for (let i = 0; i < count; i += 1) {
      state.particles.push({ x, y, type, life: 0.35 + Math.random() * 0.35, phase: Math.random() * 6.28 });
    }
  }

  function updatePickups(dt) {
    for (let i = state.pickups.length - 1; i >= 0; i -= 1) {
      const pickup = state.pickups[i];
      pickup.bob += dt * 2;
      if (Math.hypot(pickup.x - state.player.x, pickup.y - state.player.y) > 0.55) continue;
      if (pickup.type === "intel") {
        state.intel = true;
        say("Documenti recuperati. Tutto scritto bene, finalmente.", true);
        announce("Documenti recuperati.");
      } else if (pickup.type === "ammo") {
        state.player.ammo = Math.min(MAX_AMMO, state.player.ammo + 6);
        say("Banane tattiche. Munizioni perfette.", false);
      } else {
        state.player.health = Math.min(100, state.player.health + 32);
        say("Un espresso e torno indistruttibile.", false);
      }
      audio.event("pickup");
      state.pickups.splice(i, 1);
    }
  }

  function fireEnemyShot(guard, distance) {
    const player = state.player;
    const targetAngle = Math.atan2(player.y - guard.y, player.x - guard.x);
    const spread = (Math.random() - 0.5) * (0.1 + distance * 0.012 - state.levelIndex * 0.012);
    const angle = targetAngle + spread;
    const speed = 5.8 + state.levelIndex * 0.45;
    const x = guard.x + Math.cos(angle) * 0.32;
    const y = guard.y + Math.sin(angle) * 0.32;
    state.enemyShots.push({
      x, y, previousX: x, previousY: y,
      vx: Math.cos(angle) * speed,
      vy: Math.sin(angle) * speed,
      life: 1.6,
      damage: 4 + state.levelIndex
    });
    guard.muzzle = 0.11;
    audio.event("enemyShot");
  }

  function segmentDistanceToPlayer(shot) {
    const segmentX = shot.x - shot.previousX;
    const segmentY = shot.y - shot.previousY;
    const lengthSquared = segmentX * segmentX + segmentY * segmentY;
    if (lengthSquared === 0) return Math.hypot(state.player.x - shot.x, state.player.y - shot.y);
    const projection = Math.max(0, Math.min(1,
      ((state.player.x - shot.previousX) * segmentX + (state.player.y - shot.previousY) * segmentY) / lengthSquared
    ));
    const closestX = shot.previousX + segmentX * projection;
    const closestY = shot.previousY + segmentY * projection;
    return Math.hypot(state.player.x - closestX, state.player.y - closestY);
  }

  function updateEnemyShots(dt) {
    for (let i = state.enemyShots.length - 1; i >= 0; i -= 1) {
      const shot = state.enemyShots[i];
      shot.previousX = shot.x;
      shot.previousY = shot.y;
      shot.x += shot.vx * dt;
      shot.y += shot.vy * dt;
      shot.life -= dt;

      if (tileAt(shot.x, shot.y) !== 0) {
        spawnParticles(shot.previousX, shot.previousY, "enemyImpact", 4);
        state.enemyShots.splice(i, 1);
        continue;
      }

      if (segmentDistanceToPlayer(shot) < PLAYER_RADIUS + 0.08) {
        state.enemyShots.splice(i, 1);
        hurtPlayer(shot.damage);
        continue;
      }

      if (shot.life <= 0) state.enemyShots.splice(i, 1);
    }
  }

  function updateGuards(dt) {
    const player = state.player;
    for (const guard of state.guards) {
      if (guard.dead) continue;
      guard.attack -= dt;
      guard.hit = Math.max(0, guard.hit - dt);
      guard.muzzle = Math.max(0, guard.muzzle - dt);
      guard.alert = Math.max(0, guard.alert - dt);
      guard.sway += dt * 2.5;
      const dx = player.x - guard.x;
      const dy = player.y - guard.y;
      const distance = Math.hypot(dx, dy);
      const seesPlayer = distance < 7 && lineOfSight(guard.x, guard.y, player.x, player.y);
      if (seesPlayer) guard.alert = Math.max(guard.alert, 1.2);

      const preferredDistance = seesPlayer ? 3.1 : 1.35;
      if (guard.alert > 0 && distance > preferredDistance) {
        const speed = (0.48 + state.levelIndex * 0.05) * dt;
        const nextX = guard.x + dx / distance * speed;
        const nextY = guard.y + dy / distance * speed;
        if (isOpen(nextX, guard.y, 0.18)) guard.x = nextX;
        if (isOpen(guard.x, nextY, 0.18)) guard.y = nextY;
      }

      if (seesPlayer && distance < 0.82 && guard.attack <= 0) {
        guard.attack = 0.78 + Math.random() * 0.22;
        guard.muzzle = 0.08;
        audio.event("melee");
        hurtPlayer(9 + state.levelIndex * 2);
      } else if (seesPlayer && distance < 6.5 && guard.attack <= 0) {
        guard.attack = Math.max(0.72, 1.28 - state.levelIndex * 0.1) + Math.random() * 0.3;
        fireEnemyShot(guard, distance);
      }
    }
  }

  function update(dt) {
    if (state.mode !== "playing") return;
    const player = state.player;
    state.elapsed += dt;
    player.cooldown = Math.max(0, player.cooldown - dt);
    state.hitTimer = Math.max(0, state.hitTimer - dt);
    state.damageTimer = Math.max(0, state.damageTimer - dt);
    state.muzzleTimer = Math.max(0, state.muzzleTimer - dt);
    state.recoil = Math.max(0, state.recoil - dt * 7);

    if (player.reload > 0) {
      player.reload -= dt;
      if (player.reload <= 0) player.ammo = MAX_AMMO;
    }

    let forward = 0;
    let strafe = 0;
    if (input.keys.has("KeyW") || input.keys.has("ArrowUp")) forward += 1;
    if (input.keys.has("KeyS") || input.keys.has("ArrowDown")) forward -= 1;
    if (input.keys.has("KeyA")) strafe -= 1;
    if (input.keys.has("KeyD")) strafe += 1;
    forward += -input.moveY;
    strafe += input.moveX;
    if (input.keys.has("ArrowLeft")) player.angle -= 1.65 * dt;
    if (input.keys.has("ArrowRight")) player.angle += 1.65 * dt;

    const magnitude = Math.hypot(forward, strafe);
    player.moving = magnitude > 0.08;
    if (magnitude > 0) {
      forward /= Math.max(1, magnitude);
      strafe /= Math.max(1, magnitude);
      const speed = 2.35 * dt;
      const dx = (Math.cos(player.angle) * forward + Math.cos(player.angle + Math.PI / 2) * strafe) * speed;
      const dy = (Math.sin(player.angle) * forward + Math.sin(player.angle + Math.PI / 2) * strafe) * speed;
      movePlayer(dx, dy);
      player.bob += dt * 9;
    }

    if (input.fire) fireWeapon();
    updatePickups(dt);
    updateGuards(dt);
    updateEnemyShots(dt);

    for (let i = state.particles.length - 1; i >= 0; i -= 1) {
      state.particles[i].life -= dt;
      if (state.particles[i].life <= 0) state.particles.splice(i, 1);
    }

    state.dialogueTimer -= dt;
    state.randomDialogueTimer -= dt;
    if (state.dialogueTimer <= 0) ui.dialogue.classList.add("is-hidden");
    if (state.randomDialogueTimer <= 0) {
      state.randomDialogueTimer = 10 + Math.random() * 10;
      say(dialoguePool[Math.floor(Math.random() * dialoguePool.length)], false);
    }

    if (state.guards.every((guard) => guard.dead) && state.intel) completeLevel();
    updateHud();
  }

  function completeLevel() {
    if (state.mode !== "playing") return;
    state.mode = "levelComplete";
    audio.event("levelClear");
    updateHud(true);
    releasePointer();
    if (state.levelIndex < LEVELS.length - 1) {
      showMainOverlay("MISSIONE COMPIUTA", LEVELS[state.levelIndex].name, "Documenti al sicuro. La prossima operazione è già iniziata.", "PROSSIMA MISSIONE");
      say("Missione compiuta. Non avevo dubbi.", true);
    } else {
      state.mode = "victory";
      audio.event("victory");
      showMainOverlay("OPERAZIONE CONCLUSA", "Vittoria gorillesca", `Punteggio finale: ${state.score}. La giungla canta in italiano.`, "GIOCA ANCORA");
      say("Missione compiuta. La giungla canta in italiano!", true);
      updateHud(true);
    }
  }

  function advanceLevel() {
    if (state.mode !== "levelComplete") return;
    loadLevel(state.levelIndex + 1, true);
    state.mode = "playing";
    hideMainOverlay();
    audio.setPaused(false);
    updateHud(true);
    requestPointerLock();
  }

  function wallColor(level, tile, side, distance, textureX) {
    const source = tile === 2 ? level.theme.accent : level.theme.wall;
    const stripe = (Math.floor(textureX * 8) % 2) * 10;
    const mortar = textureX < 0.035 || textureX > 0.965 ? 24 : 0;
    const shade = Math.max(0.28, 1 - distance / 16) * (side ? 0.76 : 1);
    const r = Math.max(0, Math.floor((source[0] + stripe - mortar) * shade));
    const g = Math.max(0, Math.floor((source[1] + stripe - mortar) * shade));
    const b = Math.max(0, Math.floor((source[2] + stripe - mortar) * shade));
    return `rgb(${r},${g},${b})`;
  }

  function castWalls() {
    const level = LEVELS[state.levelIndex];
    const player = state.player;
    const horizon = Math.round(HEIGHT / 2 + player.pitch);
    const skyGradient = view.createLinearGradient(0, 0, 0, horizon);
    skyGradient.addColorStop(0, level.theme.sky2);
    skyGradient.addColorStop(1, level.theme.sky);
    view.fillStyle = skyGradient;
    view.fillRect(0, 0, WIDTH, horizon);
    const floorGradient = view.createLinearGradient(0, horizon, 0, HEIGHT);
    floorGradient.addColorStop(0, level.theme.floor);
    floorGradient.addColorStop(1, level.theme.floor2);
    view.fillStyle = floorGradient;
    view.fillRect(0, horizon, WIDTH, HEIGHT - horizon);

    view.globalAlpha = 0.16;
    view.strokeStyle = level.theme.accent ? `rgb(${level.theme.accent.join(",")})` : "#9f8040";
    for (let y = horizon + 12; y < HEIGHT; y += 18) {
      view.beginPath();
      view.moveTo(0, y);
      view.lineTo(WIDTH, y);
      view.stroke();
    }
    view.globalAlpha = 1;

    const dirX = Math.cos(player.angle);
    const dirY = Math.sin(player.angle);
    const planeX = -dirY * Math.tan(FOV / 2);
    const planeY = dirX * Math.tan(FOV / 2);

    for (let x = 0; x < WIDTH; x += 1) {
      const cameraX = 2 * x / WIDTH - 1;
      const rayX = dirX + planeX * cameraX;
      const rayY = dirY + planeY * cameraX;
      let mapX = Math.floor(player.x);
      let mapY = Math.floor(player.y);
      const deltaX = Math.abs(1 / (rayX || 0.00001));
      const deltaY = Math.abs(1 / (rayY || 0.00001));
      const stepX = rayX < 0 ? -1 : 1;
      const stepY = rayY < 0 ? -1 : 1;
      let sideX = rayX < 0 ? (player.x - mapX) * deltaX : (mapX + 1 - player.x) * deltaX;
      let sideY = rayY < 0 ? (player.y - mapY) * deltaY : (mapY + 1 - player.y) * deltaY;
      let side = 0;
      let tile = 0;
      let safety = 0;
      while (tile === 0 && safety < 64) {
        if (sideX < sideY) {
          sideX += deltaX;
          mapX += stepX;
          side = 0;
        } else {
          sideY += deltaY;
          mapY += stepY;
          side = 1;
        }
        tile = tileAt(mapX + 0.01, mapY + 0.01);
        safety += 1;
      }
      const distance = side === 0
        ? (mapX - player.x + (1 - stepX) / 2) / (rayX || 0.00001)
        : (mapY - player.y + (1 - stepY) / 2) / (rayY || 0.00001);
      const safeDistance = Math.max(0.001, distance);
      depthBuffer[x] = safeDistance;
      const lineHeight = Math.min(HEIGHT * 3, Math.floor(HEIGHT / safeDistance));
      const top = Math.floor(horizon - lineHeight / 2);
      let wallX = side === 0 ? player.y + safeDistance * rayY : player.x + safeDistance * rayX;
      wallX -= Math.floor(wallX);
      view.fillStyle = wallColor(level, tile, side, safeDistance, wallX);
      view.fillRect(x, Math.max(0, top), 1, Math.min(HEIGHT, lineHeight));
      if (safeDistance < 2.2 && state.muzzleTimer > 0) {
        view.fillStyle = `rgba(255,219,111,${state.muzzleTimer * 4})`;
        view.fillRect(x, Math.max(0, top), 1, Math.min(HEIGHT, lineHeight));
      }
    }
  }

  function keyOutPaleBackground(image) {
    const output = document.createElement("canvas");
    output.width = image.naturalWidth;
    output.height = image.naturalHeight;
    const outputContext = output.getContext("2d", { willReadFrequently: true });
    outputContext.drawImage(image, 0, 0);
    const pixels = outputContext.getImageData(0, 0, output.width, output.height);
    for (let i = 0; i < pixels.data.length; i += 4) {
      const r = pixels.data[i];
      const g = pixels.data[i + 1];
      const b = pixels.data[i + 2];
      const min = Math.min(r, g, b);
      const max = Math.max(r, g, b);
      if (min > 220 && max - min < 18) {
        pixels.data[i + 3] = Math.max(0, Math.min(255, (236 - min) * 16));
      }
    }
    outputContext.putImageData(pixels, 0, 0);
    return output;
  }

  function loadSprite(path) {
    const record = { ready: false, canvas: null };
    const image = new Image();
    image.onload = () => {
      record.canvas = keyOutPaleBackground(image);
      record.ready = true;
    };
    image.src = path;
    return record;
  }

  const guardSprite = loadSprite("./assets/security-guard.png");
  const handsSprite = loadSprite("./assets/gori-kongo-hands.png");

  function renderBillboards() {
    const player = state.player;
    const objects = [];
    for (const pickup of state.pickups) objects.push({ kind: "pickup", object: pickup, distance: Math.hypot(pickup.x - player.x, pickup.y - player.y) });
    for (const guard of state.guards) {
      if (!guard.dead) objects.push({ kind: "guard", object: guard, distance: Math.hypot(guard.x - player.x, guard.y - player.y) });
    }
    objects.sort((a, b) => b.distance - a.distance);

    for (const item of objects) {
      const object = item.object;
      const angle = normalizeAngle(Math.atan2(object.y - player.y, object.x - player.x) - player.angle);
      if (Math.abs(angle) > FOV * 0.68 || item.distance < 0.1) continue;
      const screenCenter = WIDTH * (0.5 + angle / FOV);
      const isGuard = item.kind === "guard";
      const size = Math.min(HEIGHT * 2, HEIGHT / item.distance * (isGuard ? 1.05 : 0.38));
      const bob = isGuard ? Math.sin(object.sway) * 1.5 : Math.sin(object.bob) * 4;
      const top = HEIGHT / 2 + player.pitch - size * (isGuard ? 0.55 : 0.05) + bob;
      const left = screenCenter - size / 2;
      const startX = Math.max(0, Math.floor(left));
      const endX = Math.min(WIDTH - 1, Math.ceil(left + size));

      if (isGuard && guardSprite.ready) {
        const sprite = guardSprite.canvas;
        for (let x = startX; x <= endX; x += 1) {
          if (item.distance >= depthBuffer[x]) continue;
          const sourceX = Math.floor((x - left) / size * sprite.width);
          view.globalAlpha = object.hit > 0 ? 0.55 : 1;
          view.drawImage(sprite, sourceX, 0, 1, sprite.height, x, top, 1, size);
        }
        view.globalAlpha = 1;
      } else if (isGuard) {
        view.fillStyle = object.hit > 0 ? "#f0ead5" : "#276c68";
        view.fillRect(left + size * 0.28, top + size * 0.2, size * 0.44, size * 0.66);
      } else {
        drawPickup(object, left, top, size, item.distance);
      }

      if (isGuard && object.muzzle > 0 && item.distance < depthBuffer[Math.max(0, Math.min(WIDTH - 1, Math.floor(screenCenter)))]) {
        const flashX = screenCenter + size * 0.18;
        const flashY = top + size * 0.55;
        const flashSize = Math.max(4, size * 0.09);
        view.save();
        view.translate(flashX, flashY);
        view.shadowColor = "#ff9f43";
        view.shadowBlur = flashSize * 1.7;
        view.fillStyle = `rgba(255,222,132,${Math.min(1, object.muzzle * 12)})`;
        view.beginPath();
        for (let point = 0; point < 10; point += 1) {
          const flashAngle = point / 10 * Math.PI * 2;
          const radius = point % 2 ? flashSize * 0.35 : flashSize;
          const x = Math.cos(flashAngle) * radius;
          const y = Math.sin(flashAngle) * radius;
          if (point === 0) view.moveTo(x, y); else view.lineTo(x, y);
        }
        view.closePath();
        view.fill();
        view.restore();
      }
    }
  }

  function drawPickup(pickup, left, top, size, distance) {
    const centerX = left + size / 2;
    const centerY = top + size / 2;
    if (centerX < 0 || centerX >= WIDTH || distance >= depthBuffer[Math.max(0, Math.min(WIDTH - 1, Math.floor(centerX)))]) return;
    view.save();
    view.translate(centerX, centerY);
    view.shadowBlur = 12;
    if (pickup.type === "intel") {
      view.shadowColor = "#5be1c3";
      view.fillStyle = "#5be1c3";
      view.fillRect(-size * 0.24, -size * 0.3, size * 0.48, size * 0.6);
      view.fillStyle = "#173f3a";
      view.fillRect(-size * 0.11, -size * 0.14, size * 0.22, size * 0.28);
    } else if (pickup.type === "ammo") {
      view.shadowColor = "#f0c84b";
      view.strokeStyle = "#f0c84b";
      view.lineWidth = Math.max(2, size * 0.12);
      view.beginPath();
      view.arc(0, -size * 0.05, size * 0.28, 0.2, 2.75);
      view.stroke();
    } else {
      view.shadowColor = "#d84a3e";
      view.fillStyle = "#f1ecd9";
      view.fillRect(-size * 0.28, -size * 0.24, size * 0.56, size * 0.48);
      view.fillStyle = "#d84a3e";
      view.fillRect(-size * 0.07, -size * 0.2, size * 0.14, size * 0.4);
      view.fillRect(-size * 0.2, -size * 0.07, size * 0.4, size * 0.14);
    }
    view.restore();
  }

  function renderParticles() {
    const player = state.player;
    for (const particle of state.particles) {
      const dx = particle.x - player.x;
      const dy = particle.y - player.y;
      const distance = Math.hypot(dx, dy);
      const angle = normalizeAngle(Math.atan2(dy, dx) - player.angle);
      if (Math.abs(angle) > FOV / 2) continue;
      const x = WIDTH * (0.5 + angle / FOV);
      const y = HEIGHT / 2 + player.pitch + Math.sin(particle.phase + state.elapsed * 12) * 7;
      view.fillStyle = particle.type === "enemyImpact"
        ? `rgba(255,113,66,${Math.min(1, particle.life * 2)})`
        : `rgba(244,196,82,${Math.min(1, particle.life * 2)})`;
      view.fillRect(x, y, 2, 2);
    }
  }

  function projectWorldPoint(x, y) {
    const dx = x - state.player.x;
    const dy = y - state.player.y;
    const distance = Math.hypot(dx, dy);
    const angle = normalizeAngle(Math.atan2(dy, dx) - state.player.angle);
    return {
      angle,
      distance,
      x: WIDTH * (0.5 + angle / FOV),
      y: HEIGHT / 2 + state.player.pitch + 3
    };
  }

  function renderEnemyShots() {
    for (const shot of state.enemyShots) {
      const current = projectWorldPoint(shot.x, shot.y);
      if (Math.abs(current.angle) > FOV * 0.58 || current.distance < 0.08) continue;
      const column = Math.max(0, Math.min(WIDTH - 1, Math.floor(current.x)));
      if (current.distance >= depthBuffer[column]) continue;

      const previous = projectWorldPoint(shot.previousX, shot.previousY);
      const radius = Math.max(1.8, Math.min(7, 8 / current.distance));
      view.save();
      view.lineCap = "round";
      view.shadowColor = "#ff6a35";
      view.shadowBlur = radius * 2.2;
      view.strokeStyle = "rgba(255,126,62,0.88)";
      view.lineWidth = radius;
      view.beginPath();
      view.moveTo(previous.x, previous.y);
      view.lineTo(current.x, current.y);
      view.stroke();
      view.fillStyle = "#fff0ad";
      view.beginPath();
      view.arc(current.x, current.y, radius * 0.55, 0, Math.PI * 2);
      view.fill();
      view.restore();
    }
  }

  function renderWeapon() {
    const player = state.player;
    const movingBob = !REDUCED_MOTION.matches && player.moving ? Math.sin(player.bob) * 3 : 0;
    const recoilY = state.recoil * 18;
    const reloadDip = player.reload > 0 ? Math.sin(Math.min(1, player.reload / 0.82) * Math.PI) * 42 : 0;
    if (handsSprite.ready) {
      const sprite = handsSprite.canvas;
      const width = Math.min(WIDTH * 0.72, 360);
      const height = width * sprite.height / sprite.width;
      view.drawImage(sprite, WIDTH / 2 - width / 2, HEIGHT - height + 34 + movingBob + recoilY + reloadDip, width, height);
    } else {
      view.fillStyle = "#241f1b";
      view.fillRect(WIDTH / 2 - 44, HEIGHT - 58 + movingBob, 88, 58);
      view.fillStyle = "#a78338";
      view.fillRect(WIDTH / 2 - 8, HEIGHT - 100 + recoilY, 16, 65);
    }

    if (state.muzzleTimer > 0) {
      view.save();
      view.translate(WIDTH / 2, HEIGHT - 118 - recoilY);
      view.fillStyle = `rgba(255,229,142,${Math.min(1, state.muzzleTimer * 16)})`;
      view.beginPath();
      for (let i = 0; i < 12; i += 1) {
        const angle = i / 12 * Math.PI * 2;
        const radius = i % 2 ? 6 : 22;
        const x = Math.cos(angle) * radius;
        const y = Math.sin(angle) * radius;
        if (i === 0) view.moveTo(x, y); else view.lineTo(x, y);
      }
      view.closePath();
      view.fill();
      view.restore();
    }
  }

  function render() {
    if (!state.player) return;
    castWalls();
    renderBillboards();
    renderEnemyShots();
    renderParticles();
    renderWeapon();

    const shake = !REDUCED_MOTION.matches && state.damageTimer > 0 ? (Math.random() - 0.5) * 12 : 0;
    const scale = Math.max(canvas.width / WIDTH, canvas.height / HEIGHT);
    const drawWidth = WIDTH * scale;
    const drawHeight = HEIGHT * scale;
    const dx = (canvas.width - drawWidth) / 2 + shake;
    const dy = (canvas.height - drawHeight) / 2 + shake * 0.5;
    context.fillStyle = "#07100d";
    context.fillRect(0, 0, canvas.width, canvas.height);
    context.drawImage(surface, dx, dy, drawWidth, drawHeight);
  }

  function updateHud(force = false) {
    if (!state.player) return;
    const alive = state.guards.filter((guard) => !guard.dead).length;
    const snapshot = `${state.mode}|${state.player.health}|${state.player.ammo}|${state.player.reload > 0}|${alive}|${state.intel}|${state.levelIndex}|${state.enemyShots.length}`;
    if (!force && snapshot === hudSnapshot) return;
    hudSnapshot = snapshot;
    ui.health.textContent = String(state.player.health);
    ui.healthBar.style.width = `${state.player.health}%`;
    ui.healthBar.style.background = state.player.health < 30 ? "var(--danger)" : "var(--green-bright)";
    ui.ammo.textContent = state.player.reload > 0 ? "--" : String(state.player.ammo);
    ui.guards.textContent = String(alive);
    ui.intel.textContent = state.intel ? "SÌ" : "NO";
    document.body.dataset.gameMode = state.mode;
    document.body.dataset.level = String(state.levelIndex + 1);
    document.body.dataset.health = String(state.player.health);
    document.body.dataset.ammo = String(state.player.ammo);
    document.body.dataset.guards = String(alive);
    document.body.dataset.intel = String(state.intel);
    document.body.dataset.enemyShots = String(state.enemyShots.length);
  }

  function resize() {
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    canvas.width = Math.max(1, Math.round(window.innerWidth * dpr));
    canvas.height = Math.max(1, Math.round(window.innerHeight * dpr));
    context.imageSmoothingEnabled = false;
  }

  function toggleMute() {
    audio.init();
    const muted = !audio.isMuted();
    audio.setMuted(muted);
    ui.muteButton.setAttribute("aria-pressed", String(muted));
    ui.muteButton.setAttribute("aria-label", muted ? "Attiva audio" : "Disattiva audio");
    ui.muteButton.textContent = muted ? "🔇" : "🔊";
  }

  function handleStartButton() {
    audio.event("button");
    if (state.mode === "levelComplete") advanceLevel();
    else startGame();
  }

  window.addEventListener("keydown", (event) => {
    const prevent = ["Space", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"].includes(event.code);
    if (prevent) event.preventDefault();
    if (event.repeat && ["KeyR", "KeyP", "KeyM", "Escape"].includes(event.code)) return;
    if (event.code === "KeyP") {
      state.mode === "paused" ? resumeGame() : pauseGame();
      return;
    }
    if (event.code === "Escape" && state.mode === "paused") {
      resumeGame();
      return;
    }
    if (event.code === "KeyM") {
      toggleMute();
      return;
    }
    if (TEST_MODE && event.code === "KeyN") {
      state.guards.forEach((guard) => { guard.dead = true; });
      state.intel = true;
      completeLevel();
      return;
    }
    if (TEST_MODE && event.code === "KeyG" && state.mode === "playing") {
      const guard = state.guards.find((candidate) => !candidate.dead);
      if (guard) {
        guard.x = state.player.x + Math.cos(state.player.angle) * 2.8;
        guard.y = state.player.y + Math.sin(state.player.angle) * 2.8;
        guard.attack = 0;
        guard.alert = 5;
      }
      return;
    }
    if (event.code === "KeyR") {
      if (state.mode === "gameOver" || state.mode === "victory") restartGame();
      else reloadWeapon();
      return;
    }
    if (event.code === "Space") {
      if (state.mode === "ready") startGame();
      else if (state.mode === "playing") {
        input.fire = true;
        fireWeapon();
      }
    }
    input.keys.add(event.code);
  });

  window.addEventListener("keyup", (event) => {
    input.keys.delete(event.code);
    if (event.code === "Space") input.fire = false;
  });

  window.addEventListener("blur", () => {
    if (state.mode === "playing") pauseGame();
    clearInputs();
  });

  document.addEventListener("visibilitychange", () => {
    if (document.hidden && state.mode === "playing") pauseGame();
  });

  document.addEventListener("pointerlockchange", () => {
    if (suppressPointerPause) {
      suppressPointerPause = false;
      return;
    }
    if (document.pointerLockElement !== canvas && state.mode === "playing" && matchMedia("(pointer: fine)").matches) {
      pauseGame(true);
    }
  });

  document.addEventListener("mousemove", (event) => {
    if (document.pointerLockElement !== canvas || state.mode !== "playing") return;
    state.player.angle += event.movementX * 0.0026;
    state.player.pitch = Math.max(-42, Math.min(42, state.player.pitch + event.movementY * 0.09));
  });

  canvas.addEventListener("mousedown", (event) => {
    if (event.button !== 0) return;
    audio.init();
    if (state.mode === "ready") startGame();
    else if (state.mode === "playing") {
      if (document.pointerLockElement !== canvas) requestPointerLock();
      input.fire = true;
      fireWeapon();
    }
  });

  window.addEventListener("mouseup", () => { input.fire = false; });
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());
  ui.startButton.addEventListener("click", handleStartButton);
  ui.pauseButton.addEventListener("click", () => state.mode === "paused" ? resumeGame() : pauseGame());
  ui.resumeButton.addEventListener("click", resumeGame);
  ui.restartButton.addEventListener("click", restartGame);
  ui.muteButton.addEventListener("click", toggleMute);

  function updateMovePad(event) {
    const rect = ui.movePad.getBoundingClientRect();
    const dx = event.clientX - (rect.left + rect.width / 2);
    const dy = event.clientY - (rect.top + rect.height / 2);
    const radius = rect.width * 0.34;
    const length = Math.hypot(dx, dy) || 1;
    const scale = Math.min(1, radius / length);
    const x = dx * scale;
    const y = dy * scale;
    input.moveX = x / radius;
    input.moveY = y / radius;
    ui.moveStick.style.transform = `translate(calc(-50% + ${x}px), calc(-50% + ${y}px))`;
  }

  ui.movePad.addEventListener("pointerdown", (event) => {
    audio.init();
    input.movePointer = event.pointerId;
    ui.movePad.setPointerCapture(event.pointerId);
    updateMovePad(event);
  });
  ui.movePad.addEventListener("pointermove", (event) => {
    if (event.pointerId === input.movePointer) updateMovePad(event);
  });
  const endMove = (event) => {
    if (event.pointerId !== input.movePointer) return;
    input.movePointer = null;
    input.moveX = 0;
    input.moveY = 0;
    ui.moveStick.style.transform = "translate(-50%, -50%)";
  };
  ui.movePad.addEventListener("pointerup", endMove);
  ui.movePad.addEventListener("pointercancel", endMove);

  ui.lookZone.addEventListener("pointerdown", (event) => {
    audio.init();
    input.lookPointer = event.pointerId;
    input.lookX = event.clientX;
    input.lookY = event.clientY;
    ui.lookZone.setPointerCapture(event.pointerId);
  });
  ui.lookZone.addEventListener("pointermove", (event) => {
    if (event.pointerId !== input.lookPointer || state.mode !== "playing") return;
    state.player.angle += (event.clientX - input.lookX) * 0.009;
    state.player.pitch = Math.max(-42, Math.min(42, state.player.pitch + (event.clientY - input.lookY) * 0.18));
    input.lookX = event.clientX;
    input.lookY = event.clientY;
  });
  const endLook = (event) => { if (event.pointerId === input.lookPointer) input.lookPointer = null; };
  ui.lookZone.addEventListener("pointerup", endLook);
  ui.lookZone.addEventListener("pointercancel", endLook);

  ui.touchFire.addEventListener("pointerdown", (event) => {
    event.preventDefault();
    audio.init();
    input.fire = true;
    fireWeapon();
    ui.touchFire.setPointerCapture(event.pointerId);
  });
  const stopTouchFire = () => { input.fire = false; };
  ui.touchFire.addEventListener("pointerup", stopTouchFire);
  ui.touchFire.addEventListener("pointercancel", stopTouchFire);
  ui.touchReload.addEventListener("click", reloadWeapon);
  window.addEventListener("resize", resize);

  function loop(now) {
    const delta = Math.min(0.1, (now - lastTime) / 1000);
    lastTime = now;
    if (state.mode === "playing") {
      accumulator += delta;
      let steps = 0;
      while (accumulator >= FIXED_STEP && steps < MAX_STEPS) {
        update(FIXED_STEP);
        accumulator -= FIXED_STEP;
        steps += 1;
      }
    } else {
      accumulator = 0;
    }
    render();
    requestAnimationFrame(loop);
  }

  if (TEST_MODE) {
    window.__GORILLA_TEST__ = {
      getState: () => ({
        mode: state.mode, levelIndex: state.levelIndex, health: state.player.health, ammo: state.player.ammo,
        x: state.player.x, y: state.player.y, angle: state.player.angle,
        guardsAlive: state.guards.filter((guard) => !guard.dead).length, enemyShots: state.enemyShots.length,
        intel: state.intel, score: state.score
      }),
      start: startGame,
      damagePlayer: hurtPlayer,
      completeObjective: () => {
        state.guards.forEach((guard) => { guard.dead = true; });
        state.intel = true;
        completeLevel();
      },
      spawnAttacker: () => {
        const guard = state.guards.find((candidate) => !candidate.dead);
        if (!guard) return false;
        guard.x = state.player.x + Math.cos(state.player.angle) * 2.8;
        guard.y = state.player.y + Math.sin(state.player.angle) * 2.8;
        guard.attack = 0;
        guard.alert = 5;
        return true;
      },
      advanceLevel,
      restart: restartGame
    };
  }

  resize();
  loadLevel(0, false);
  ui.muteButton.setAttribute("aria-pressed", String(audio.isMuted()));
  requestAnimationFrame(loop);
})();
