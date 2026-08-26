import * as THREE from "./vendor/three.module.min.js";

(() => {
  "use strict";

  const canvas = document.getElementById("game");
  const gameShell = canvas.closest(".game-shell");
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
    touchReload: document.getElementById("touchReload"),
  };

  const audio = window.GorillaAudio || {
    init() {}, setMuted() {}, isMuted() { return true; }, setPaused() {},
    event() {}, speakItalian() {}, stopSpeech() {},
  };

  const FIXED_STEP = 1 / 60;
  const MAX_STEPS = 5;
  const PLAYER_HEIGHT = 1.62;
  const PLAYER_RADIUS = 0.22;
  const MAX_AMMO = 12;
  const FOV = 72;
  const TEST_MODE = new URLSearchParams(location.search).has("e2e");
  const REDUCED_MOTION = matchMedia("(prefers-reduced-motion: reduce)").matches;
  const MOBILE = matchMedia("(pointer: coarse), (hover: none), (any-pointer: coarse)").matches;

  const LEVELS = [
    {
      name: "PORTO NEBBIA",
      objective: "Recupera il manifesto cifrato e neutralizza le guardie.",
      theme: { sky: 0x091921, fog: 0x173c40, floor: 0x182724, wall: 0x32675d, accent: 0xd79b3a, light: 0x69e6c7 },
      spawn: [1.5, 1.5, 0], guards: 6,
      map: [
        "1111111111111111", "1000001000000001", "1011001022201101", "1010000000200101",
        "1010111110200101", "1000100010000101", "1110101011110101", "1000001000010001",
        "1022201111011101", "1000200001000101", "1010201101010101", "1010000100010101",
        "1011110111010101", "1000010001000001", "1011000100011001", "1111111111111111",
      ],
    },
    {
      name: "VILLA VULCANO",
      objective: "Sottrai il microfilm dalla sala comunicazioni.",
      theme: { sky: 0x21100d, fog: 0x4c2922, floor: 0x2f2922, wall: 0x817657, accent: 0xc44f35, light: 0xffc873 },
      spawn: [1.5, 1.5, 0], guards: 8,
      map: [
        "1111111111111111", "1000000010000001", "1011101010111101", "1000101000100001",
        "1110101110101101", "1000100010001001", "1011111011101011", "1000001000100001",
        "1011101110111101", "1010000010000101", "1010111011110101", "1000100000100001",
        "1110101110111101", "1000100010000001", "1010001100111001", "1111111111111111",
      ],
    },
    {
      name: "STAZIONE ORSO",
      objective: "Recupera il prototipo e chiudi l'operazione.",
      theme: { sky: 0x07111d, fog: 0x26394d, floor: 0x202b35, wall: 0x536c7e, accent: 0xcf3d34, light: 0x8ecbff },
      spawn: [1.5, 1.5, 0], guards: 10,
      map: [
        "1111111111111111", "1000100000100001", "1010101110101101", "1010000010001001",
        "1011111011101011", "1000001000101001", "1111101110101101", "1000100010000101",
        "1010111011110101", "1010001000010001", "1011101111011101", "1000100001000101",
        "1110111101110101", "1000001000000001", "1011100011111101", "1111111111111111",
      ],
    },
  ];

  const dialoguePool = [
    "Andiamo. La notte è giovane e io ho fame.",
    "Questa faccenda puzza. E non sono stato io.",
    "Nessuno tocca le banane dell'agente Kongo.",
    "Eleganza, precisione, pelo impeccabile.",
    "Parlo piano, ma le mie mani fanno rumore.",
    "Un gorilla, una missione, dodici colpi. Basta e avanza.",
  ];

  const state = {
    mode: "ready", levelIndex: 0, map: [], player: null,
    guards: [], pickups: [], enemyShots: [], effects: [], obstacles: [], intel: false,
    score: 0, elapsed: 0, dialogueTimer: 4, randomDialogueTimer: 9,
    damageTimer: 0, muzzleTimer: 0, recoil: 0, aiming: false,
    extraction: false, extractionView: null,
  };

  const input = {
    keys: new Set(), fire: false, moveX: 0, moveY: 0,
    movePointer: null, lookPointer: null, lookX: 0, lookY: 0,
  };

  let renderer;
  let scene;
  let camera;
  let viewmodelScene;
  let viewmodelCamera;
  let world;
  let effectRoot;
  let weaponView;
  let weaponFlash;
  let weaponLight;
  let hemisphereLight;
  let keyLight;
  let lastTime = performance.now();
  let accumulator = 0;
  let suppressPointerPause = false;
  let hudSnapshot = "";
  let qualityScale = MOBILE ? 0.8 : 1;
  let slowFrames = 0;
  const raycaster = new THREE.Raycaster();
  const centerScreen = new THREE.Vector2(0, 0);
  const tempVector = new THREE.Vector3();
  const tempVector2 = new THREE.Vector3();
  const yAxis = new THREE.Vector3(0, 1, 0);
  const sharedEffectGeometry = new THREE.IcosahedronGeometry(0.035, 0);
  const effectMaterials = new Map();

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

  function tileAt(x, z) {
    const mx = Math.floor(x);
    const mz = Math.floor(z);
    if (mz < 0 || mz >= state.map.length || mx < 0 || mx >= state.map[0].length) return 1;
    return Number(state.map[mz][mx]);
  }

  function isOpen(x, z, radius = 0) {
    return tileAt(x - radius, z - radius) === 0 && tileAt(x + radius, z - radius) === 0 &&
      tileAt(x - radius, z + radius) === 0 && tileAt(x + radius, z + radius) === 0 &&
      !hitsObstacle(x, z, radius);
  }

  function hitsObstacle(x, z, radius = 0, y = null) {
    return state.obstacles.some((obstacle) =>
      (y === null || y <= obstacle.height) &&
      Math.hypot(x - obstacle.x, z - obstacle.z) < obstacle.radius + radius);
  }

  function openCells() {
    const cells = [];
    for (let z = 1; z < state.map.length - 1; z += 1) {
      for (let x = 1; x < state.map[z].length - 1; x += 1) {
        if (Number(state.map[z][x]) === 0) cells.push({ x: x + 0.5, z: z + 0.5 });
      }
    }
    return cells;
  }

  function lineOfSight(x1, z1, x2, z2) {
    const distance = Math.hypot(x2 - x1, z2 - z1);
    const steps = Math.max(1, Math.ceil(distance * 14));
    for (let i = 1; i < steps; i += 1) {
      const t = i / steps;
      if (tileAt(x1 + (x2 - x1) * t, z1 + (z2 - z1) * t) !== 0) return false;
    }
    return true;
  }

  function makePatternTexture(primary, secondary, accent, type) {
    const textureCanvas = document.createElement("canvas");
    textureCanvas.width = 128;
    textureCanvas.height = 128;
    const context = textureCanvas.getContext("2d");
    context.fillStyle = primary;
    context.fillRect(0, 0, 128, 128);
    context.fillStyle = secondary;
    if (type === "floor") {
      for (let y = 0; y < 128; y += 32) {
        for (let x = 0; x < 128; x += 32) {
          if ((x + y) % 64 === 0) context.fillRect(x, y, 32, 32);
        }
      }
      context.strokeStyle = accent;
      context.globalAlpha = 0.22;
      for (let p = 0; p <= 128; p += 32) {
        context.beginPath(); context.moveTo(p, 0); context.lineTo(p, 128); context.stroke();
        context.beginPath(); context.moveTo(0, p); context.lineTo(128, p); context.stroke();
      }
    } else {
      for (let y = 0; y < 128; y += 24) context.fillRect(0, y, 128, 3);
      context.fillStyle = accent;
      context.globalAlpha = 0.3;
      context.fillRect(11, 0, 3, 128);
      context.fillRect(112, 0, 2, 128);
      context.globalAlpha = 0.16;
      for (let y = 8; y < 128; y += 24) context.fillRect(0, y, 128, 1);
    }
    const texture = new THREE.CanvasTexture(textureCanvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
    texture.anisotropy = Math.min(8, renderer.capabilities.getMaxAnisotropy());
    return texture;
  }

  function colorStyle(hex) {
    return `#${hex.toString(16).padStart(6, "0")}`;
  }

  function disposeObjectResources(group) {
    const geometries = new Set();
    const materials = new Set();
    const textures = new Set();
    group.traverse((object) => {
      if (object.geometry) geometries.add(object.geometry);
      const list = Array.isArray(object.material) ? object.material : [object.material];
      for (const material of list) {
        if (!material) continue;
        materials.add(material);
        if (material.map) textures.add(material.map);
      }
    });
    geometries.forEach((geometry) => geometry.dispose());
    materials.forEach((material) => material.dispose());
    textures.forEach((texture) => texture.dispose());
  }

  function disposeGroup(group) {
    disposeObjectResources(group);
    scene.remove(group);
  }

  function makeStandard(color, options = {}) {
    return new THREE.MeshStandardMaterial({
      color,
      roughness: options.roughness ?? 0.58,
      metalness: options.metalness ?? 0.28,
      emissive: options.emissive ?? 0x000000,
      emissiveIntensity: options.emissiveIntensity ?? 0,
      map: options.map || null,
    });
  }

  function buildLevelWorld(level) {
    if (world) disposeGroup(world);
    world = new THREE.Group();
    world.name = "mission-world";
    scene.add(world);
    effectRoot.clear();
    state.effects.length = 0;
    state.obstacles = [];

    scene.background = new THREE.Color(level.theme.sky);
    scene.fog = new THREE.FogExp2(level.theme.fog, 0.055);
    hemisphereLight.color.set(level.theme.light);
    hemisphereLight.groundColor.set(level.theme.floor);
    keyLight.color.set(level.theme.light);

    const floorMap = makePatternTexture(colorStyle(level.theme.floor), "#101917", colorStyle(level.theme.accent), "floor");
    floorMap.repeat.set(8, 8);
    const wallMap = makePatternTexture(colorStyle(level.theme.wall), "#142723", colorStyle(level.theme.light), "wall");
    const accentMap = makePatternTexture(colorStyle(level.theme.accent), "#351510", "#ffcf74", "wall");
    const floorMaterial = makeStandard(0xffffff, { map: floorMap, roughness: 0.86, metalness: 0.18 });
    const ceilingMaterial = makeStandard(level.theme.sky, { roughness: 0.92, metalness: 0.05 });
    const wallMaterial = makeStandard(0xffffff, { map: wallMap, roughness: 0.52, metalness: 0.38 });
    const accentMaterial = makeStandard(0xffffff, {
      map: accentMap, roughness: 0.42, metalness: 0.48,
      emissive: level.theme.accent, emissiveIntensity: 0.14,
    });

    const floor = new THREE.Mesh(new THREE.PlaneGeometry(16, 16), floorMaterial);
    floor.rotation.x = -Math.PI / 2;
    floor.position.set(8, 0, 8);
    floor.receiveShadow = true;
    floor.userData.worldSolid = true;
    world.add(floor);
    const ceiling = new THREE.Mesh(new THREE.PlaneGeometry(16, 16), ceilingMaterial);
    ceiling.rotation.x = Math.PI / 2;
    ceiling.position.set(8, 3.2, 8);
    ceiling.userData.worldSolid = true;
    world.add(ceiling);

    const normalWalls = [];
    const accentWalls = [];
    for (let z = 0; z < state.map.length; z += 1) {
      for (let x = 0; x < state.map[z].length; x += 1) {
        const tile = Number(state.map[z][x]);
        if (tile === 1) normalWalls.push([x + 0.5, z + 0.5]);
        else if (tile === 2) accentWalls.push([x + 0.5, z + 0.5]);
      }
    }
    const wallGeometry = new THREE.BoxGeometry(1, 3.2, 1);
    const matrix = new THREE.Matrix4();
    for (const [cells, material] of [[normalWalls, wallMaterial], [accentWalls, accentMaterial]]) {
      if (cells.length === 0) continue;
      const mesh = new THREE.InstancedMesh(wallGeometry, material, cells.length);
      cells.forEach(([x, z], index) => {
        matrix.makeTranslation(x, 1.6, z);
        mesh.setMatrixAt(index, matrix);
      });
      mesh.castShadow = !MOBILE;
      mesh.receiveShadow = true;
      mesh.userData.worldSolid = true;
      world.add(mesh);
    }

    const cells = openCells();
    const fixtureMaterial = makeStandard(level.theme.light, {
      emissive: level.theme.light, emissiveIntensity: 2.4, roughness: 0.3,
    });
    cells.filter((_cell, index) => index % 23 === 7).slice(0, MOBILE ? 2 : 5).forEach((cell) => {
      const fixture = new THREE.Mesh(new THREE.BoxGeometry(0.52, 0.05, 0.16), fixtureMaterial);
      fixture.position.set(cell.x, 3.05, cell.z);
      world.add(fixture);
      if (!MOBILE) {
        const light = new THREE.PointLight(level.theme.light, 2.4, 5.5, 2);
        light.position.set(cell.x, 2.75, cell.z);
        world.add(light);
      }
    });

    const crateMaterial = makeStandard(0x293b34, { roughness: 0.68, metalness: 0.42 });
    cells.filter((_cell, index) => index % 31 === 12).slice(0, 8).forEach((cell, index) => {
      const crate = new THREE.Mesh(new THREE.BoxGeometry(0.38, 0.38, 0.38), crateMaterial);
      const crateX = cell.x + (index % 2 ? 0.27 : -0.27);
      const crateZ = cell.z + 0.27;
      crate.position.set(crateX, 0.19, crateZ);
      crate.rotation.y = index * 0.77;
      crate.castShadow = !MOBILE;
      crate.receiveShadow = true;
      crate.userData.worldSolid = true;
      world.add(crate);
      state.obstacles.push({ x: crateX, z: crateZ, radius: 0.27, height: 0.38 });
    });
  }

  function markGuardMesh(mesh, guard, zone) {
    mesh.userData.guard = guard;
    mesh.userData.hitZone = zone;
    mesh.castShadow = !MOBILE;
    mesh.receiveShadow = true;
    return mesh;
  }

  function createGuardView(guard) {
    const group = new THREE.Group();
    const uniform = makeStandard(0x254d4b, { roughness: 0.52, metalness: 0.32 });
    const armor = makeStandard(0x10181b, { roughness: 0.38, metalness: 0.7 });
    const skin = makeStandard(0xb17b58, { roughness: 0.76, metalness: 0.02 });
    const dark = makeStandard(0x0b1011, { roughness: 0.48, metalness: 0.54 });
    guard.materials = [uniform, armor, skin, dark];

    const torso = markGuardMesh(new THREE.Mesh(new THREE.CapsuleGeometry(0.28, 0.48, 5, 8), uniform), guard, "body");
    torso.position.y = 1.08;
    torso.scale.set(1, 1, 0.72);
    group.add(torso);
    if (!MOBILE) {
      const vest = markGuardMesh(new THREE.Mesh(new THREE.BoxGeometry(0.54, 0.52, 0.24), armor), guard, "body");
      vest.position.set(0, 1.15, 0.16);
      group.add(vest);
    }
    const head = markGuardMesh(new THREE.Mesh(new THREE.SphereGeometry(0.2, 12, 8), skin), guard, "head");
    head.position.y = 1.68;
    group.add(head);
    if (!MOBILE) {
      const helmet = markGuardMesh(new THREE.Mesh(new THREE.SphereGeometry(0.215, 12, 6, 0, Math.PI * 2, 0, Math.PI * 0.55), dark), guard, "head");
      helmet.position.y = 1.73;
      group.add(helmet);
    }

    guard.limbs = [];
    guard.legs = [];
    for (const side of [-1, 1]) {
      if (!MOBILE) {
        const arm = markGuardMesh(new THREE.Mesh(new THREE.CapsuleGeometry(0.075, 0.43, 3, 6), uniform), guard, "body");
        arm.position.set(side * 0.34, 1.12, 0.02);
        arm.rotation.z = side * 0.16;
        group.add(arm);
        guard.limbs.push(arm);
      }
      const leg = markGuardMesh(new THREE.Mesh(new THREE.CapsuleGeometry(0.095, 0.5, 3, 6), dark), guard, "body");
      leg.position.set(side * 0.14, 0.43, 0);
      group.add(leg);
      guard.limbs.push(leg);
      guard.legs.push(leg);
    }

    const gun = new THREE.Group();
    const receiver = new THREE.Mesh(new THREE.BoxGeometry(0.13, 0.12, 0.58), dark);
    receiver.position.z = 0.28;
    gun.add(receiver);
    if (!MOBILE) {
      const barrel = new THREE.Mesh(new THREE.CylinderGeometry(0.025, 0.025, 0.46, 7), armor);
      barrel.rotation.x = Math.PI / 2;
      barrel.position.z = 0.72;
      gun.add(barrel);
    }
    gun.position.set(0.18, 1.15, 0.34);
    group.add(gun);
    const flash = new THREE.Mesh(
      new THREE.IcosahedronGeometry(0.11, 0),
      new THREE.MeshBasicMaterial({ color: 0xffd27a, transparent: true, opacity: 0.95 }),
    );
    flash.position.set(0, 0, 0.98);
    flash.visible = false;
    gun.add(flash);
    const alert = new THREE.Mesh(
      new THREE.OctahedronGeometry(0.1, 0),
      new THREE.MeshBasicMaterial({ color: 0xffb33f }),
    );
    alert.position.y = 2.12;
    alert.visible = false;
    group.add(alert);
    guard.flash = flash;
    guard.alertIcon = alert;
    guard.view = group;
    world.add(group);
    return group;
  }

  function createPickupView(pickup) {
    const group = new THREE.Group();
    let mesh;
    if (pickup.type === "intel") {
      const material = makeStandard(0x62f5d0, { emissive: 0x25c9b0, emissiveIntensity: 2, metalness: 0.4 });
      mesh = new THREE.Mesh(new THREE.BoxGeometry(0.42, 0.08, 0.32), material);
      const stripe = new THREE.Mesh(new THREE.BoxGeometry(0.12, 0.085, 0.34), makeStandard(0x0b3633, { metalness: 0.6 }));
      mesh.add(stripe);
    } else if (pickup.type === "ammo") {
      const material = makeStandard(0xe6bd3b, { emissive: 0x7a5500, emissiveIntensity: 0.8, metalness: 0.65 });
      mesh = new THREE.Mesh(new THREE.TorusGeometry(0.22, 0.065, 8, 18, Math.PI * 1.45), material);
      mesh.rotation.x = Math.PI / 2;
    } else {
      const white = makeStandard(0xdde6df, { roughness: 0.58 });
      mesh = new THREE.Mesh(new THREE.BoxGeometry(0.38, 0.28, 0.18), white);
      const crossMaterial = makeStandard(0xd33f35, { emissive: 0x7a1010, emissiveIntensity: 0.5 });
      const vertical = new THREE.Mesh(new THREE.BoxGeometry(0.08, 0.23, 0.19), crossMaterial);
      const horizontal = new THREE.Mesh(new THREE.BoxGeometry(0.22, 0.08, 0.19), crossMaterial);
      mesh.add(vertical, horizontal);
    }
    mesh.castShadow = !MOBILE;
    group.add(mesh);
    if (!MOBILE) {
      const glow = new THREE.PointLight(
        pickup.type === "intel" ? 0x4cf0cb : pickup.type === "ammo" ? 0xf0c84b : 0xe95a4b,
        1.4, 2.2, 2,
      );
      group.add(glow);
    }
    group.position.set(pickup.x, 0.6, pickup.z);
    pickup.view = group;
    world.add(group);
  }

  function createWeaponView() {
    const group = new THREE.Group();
    const fur = makeStandard(0x2b211c, { roughness: 0.94, metalness: 0.01 });
    const skin = makeStandard(0x4d3528, { roughness: 0.86, metalness: 0.02 });
    const metal = makeStandard(0x171d20, { roughness: 0.28, metalness: 0.9 });
    const gold = makeStandard(0xa57b2a, { roughness: 0.3, metalness: 0.88 });
    const receiver = new THREE.Mesh(new THREE.BoxGeometry(0.18, 0.2, 0.62), metal);
    receiver.position.z = -0.13;
    group.add(receiver);
    const slide = new THREE.Mesh(new THREE.BoxGeometry(0.16, 0.11, 0.7), gold);
    slide.position.set(0, 0.1, -0.18);
    group.add(slide);
    const barrel = new THREE.Mesh(new THREE.CylinderGeometry(0.035, 0.04, 0.45, 10), metal);
    barrel.rotation.x = Math.PI / 2;
    barrel.position.z = -0.62;
    group.add(barrel);
    const grip = new THREE.Mesh(new THREE.BoxGeometry(0.16, 0.38, 0.2), metal);
    grip.position.set(0, -0.25, 0.02);
    grip.rotation.x = -0.18;
    group.add(grip);
    for (const side of [-1, 1]) {
      const hand = new THREE.Mesh(new THREE.SphereGeometry(0.18, 12, 8), skin);
      hand.scale.set(1.25, 0.8, 1.35);
      hand.position.set(side * 0.18, -0.23, side < 0 ? 0.04 : -0.18);
      group.add(hand);
      const forearm = new THREE.Mesh(new THREE.CapsuleGeometry(0.13, 0.45, 4, 8), fur);
      forearm.rotation.x = Math.PI / 2.5;
      forearm.position.set(side * 0.31, -0.42, 0.25);
      group.add(forearm);
    }
    weaponFlash = new THREE.Mesh(
      new THREE.IcosahedronGeometry(0.12, 0),
      new THREE.MeshBasicMaterial({ color: 0xffdf83, transparent: true, opacity: 0.95 }),
    );
    weaponFlash.position.z = -0.88;
    weaponFlash.visible = false;
    group.add(weaponFlash);
    weaponLight = new THREE.PointLight(0xffb04a, 0, 2.4, 2);
    weaponLight.position.z = -0.85;
    group.add(weaponLight);
    group.position.set(0.3, -0.33, -0.58);
    group.rotation.set(-0.05, -0.07, 0);
    viewmodelCamera.add(group);
    weaponView = group;
  }

  function initializeRenderer() {
    try {
      renderer = new THREE.WebGLRenderer({
        canvas, antialias: !MOBILE, alpha: false, powerPreference: "high-performance",
      });
    } catch (_error) {
      ui.overlayKicker.textContent = "WEBGL NON DISPONIBILE";
      ui.overlayTitle.textContent = "Motore 3D non avviato";
      ui.overlayText.textContent = "Apri il gioco in un browser moderno con accelerazione grafica attiva.";
      ui.startButton.hidden = true;
      return false;
    }
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.ACESFilmicToneMapping;
    renderer.toneMappingExposure = 1.18;
    renderer.shadowMap.enabled = !MOBILE;
    renderer.shadowMap.type = THREE.PCFShadowMap;
    canvas.addEventListener("webglcontextlost", (event) => {
      event.preventDefault();
      if (state.mode === "playing") pauseGame();
      announce("Contesto grafico sospeso. Ripristino in corso.");
    });
    canvas.addEventListener("webglcontextrestored", () => location.reload());
    scene = new THREE.Scene();
    camera = new THREE.PerspectiveCamera(FOV, 1, 0.025, 40);
    scene.add(camera);
    viewmodelScene = new THREE.Scene();
    viewmodelCamera = new THREE.PerspectiveCamera(50, 1, 0.01, 10);
    viewmodelScene.add(viewmodelCamera);
    const viewmodelFill = new THREE.HemisphereLight(0xffe9c5, 0x18221f, 2.4);
    const viewmodelKey = new THREE.DirectionalLight(0xffc66d, 3.2);
    viewmodelKey.position.set(-2, 3, 4);
    viewmodelScene.add(viewmodelFill, viewmodelKey);
    effectRoot = new THREE.Group();
    scene.add(effectRoot);
    hemisphereLight = new THREE.HemisphereLight(0x8cebd7, 0x18211f, 1.8);
    scene.add(hemisphereLight);
    keyLight = new THREE.DirectionalLight(0xffe2a6, 2.2);
    keyLight.position.set(4, 8, 2);
    keyLight.castShadow = !MOBILE;
    keyLight.shadow.mapSize.set(MOBILE ? 512 : 1024, MOBILE ? 512 : 1024);
    keyLight.shadow.camera.left = -10;
    keyLight.shadow.camera.right = 10;
    keyLight.shadow.camera.top = 10;
    keyLight.shadow.camera.bottom = -10;
    scene.add(keyLight);
    createWeaponView();
    resize();
    return true;
  }

  function loadLevel(index, preserveVitals = false) {
    const level = LEVELS[index];
    const oldHealth = state.player ? state.player.health : 100;
    const oldAmmo = state.player ? state.player.ammo : MAX_AMMO;
    state.levelIndex = index;
    state.map = level.map.slice();
    state.player = {
      x: level.spawn[0], z: level.spawn[1], angle: level.spawn[2], pitch: 0,
      health: preserveVitals ? Math.min(100, oldHealth + 20) : 100,
      ammo: preserveVitals ? Math.max(6, oldAmmo) : MAX_AMMO,
      cooldown: 0, hurtCooldown: 0, reload: 0, bob: 0, moving: false, sprinting: false,
      stepTimer: 0,
    };
    state.guards = [];
    state.pickups = [];
    state.enemyShots = [];
    state.intel = false;
    state.extraction = false;
    state.extractionView = null;
    state.damageTimer = 0;
    state.muzzleTimer = 0;
    state.recoil = 0;
    buildLevelWorld(level);

    const cells = openCells().filter((cell) => !hitsObstacle(cell.x, cell.z, 0.35));
    const random = seededRandom(8128 + index * 997);
    const available = cells.filter((cell) => Math.hypot(cell.x - state.player.x, cell.z - state.player.z) > 4);
    for (let i = 0; i < level.guards && available.length; i += 1) {
      const choice = Math.floor(random() * available.length);
      const cell = available.splice(choice, 1)[0];
      const guard = {
        x: cell.x, z: cell.z, health: index === 2 && i === 0 ? 4 : 2,
        dead: false, death: 0, hit: 0, muzzle: 0, suspicion: 0,
        ai: "patrol", attack: 0.5 + random(), telegraph: 0, burstGap: 0,
        burstRemaining: 0, stagger: 0, alert: 0, nearMiss: 0,
        patrolAngle: random() * Math.PI * 2, patrolTimer: 1 + random() * 2,
        strafe: random() < 0.5 ? -1 : 1, sway: random() * Math.PI * 2,
        lastSeenX: cell.x, lastSeenZ: cell.z, materials: [], limbs: [],
      };
      createGuardView(guard);
      guard.view.position.set(guard.x, 0, guard.z);
      state.guards.push(guard);
    }

    const farthest = cells.slice().sort((a, b) =>
      Math.hypot(b.x - state.player.x, b.z - state.player.z) - Math.hypot(a.x - state.player.x, a.z - state.player.z));
    const pickupData = [{ ...farthest[0], type: "intel", phase: 0 }];
    const supplies = farthest.filter((_cell, i) => i > 5 && i % 7 === 0).slice(0, 3);
    if (supplies[0]) pickupData.push({ ...supplies[0], type: "ammo", phase: 1.4 });
    if (supplies[1]) pickupData.push({ ...supplies[1], type: "health", phase: 2.6 });
    if (supplies[2]) pickupData.push({ ...supplies[2], type: "ammo", phase: 3.8 });
    pickupData.forEach((pickup) => {
      createPickupView(pickup);
      state.pickups.push(pickup);
    });

    ui.levelName.textContent = level.name;
    ui.objective.textContent = level.objective;
    say(index === 0 ? "Agente Kongo al rapporto. Parlo solo italiano." : "Nuova missione. Stesso gorilla. Andiamo.", true);
    updateCamera(0);
    updateHud(true);
  }

  function clearInputs() {
    input.keys.clear(); input.fire = false; input.moveX = 0; input.moveY = 0;
    input.movePointer = null; input.lookPointer = null; state.aiming = false;
    ui.moveStick.style.transform = "translate(-50%, -50%)";
  }

  function showMainOverlay(kicker, title, text, button) {
    ui.overlayKicker.textContent = kicker;
    ui.overlayTitle.textContent = title;
    ui.overlayText.textContent = text;
    ui.startButton.textContent = button;
    ui.startButton.hidden = false;
    ui.overlay.classList.add("is-visible");
    setTimeout(() => ui.startButton.focus(), 30);
  }

  function hideMainOverlay() {
    ui.overlay.classList.remove("is-visible");
    ui.startButton.blur();
    gameShell.scrollTop = 0;
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

  function say(text, speak = false) {
    ui.dialogueText.textContent = `“${text}”`;
    ui.dialogue.classList.remove("is-hidden");
    state.dialogueTimer = 4.2;
    if (speak) audio.speakItalian(text);
  }

  function announce(text) { ui.statusRegion.textContent = text; }

  function requestPointerLock() {
    if (matchMedia("(pointer: fine)").matches && canvas.requestPointerLock && document.pointerLockElement !== canvas) {
      const request = canvas.requestPointerLock();
      if (request && typeof request.catch === "function") request.catch(() => {});
    }
  }

  function releasePointer() {
    if (document.pointerLockElement === canvas) {
      suppressPointerPause = true;
      document.exitPointerLock();
    }
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
    clearInputs(); state.score = 0; loadLevel(0, false); state.mode = "playing";
    hidePauseOverlay(); hideMainOverlay(); audio.setPaused(false); updateHud(true);
    audio.event("button"); requestPointerLock();
  }

  function pauseGame(fromPointer = false) {
    if (state.mode !== "playing") return;
    state.mode = "paused"; clearInputs(); audio.setPaused(true); updateHud(true); showPauseOverlay();
    if (!fromPointer && document.pointerLockElement === canvas) {
      suppressPointerPause = true; document.exitPointerLock();
    }
    announce("Missione in pausa.");
  }

  function resumeGame() {
    if (state.mode !== "paused") return;
    audio.init(); audio.setPaused(false); state.mode = "playing"; hidePauseOverlay();
    lastTime = performance.now(); accumulator = 0; updateHud(true); requestPointerLock();
  }

  function movePlayer(dx, dz) {
    const nextX = state.player.x + dx;
    const nextZ = state.player.z + dz;
    const clearsGuards = (x, z) => state.guards.every((guard) =>
      guard.dead || Math.hypot(x - guard.x, z - guard.z) > PLAYER_RADIUS + 0.22);
    if (isOpen(nextX, state.player.z, PLAYER_RADIUS) && clearsGuards(nextX, state.player.z)) {
      state.player.x = nextX;
    }
    if (isOpen(state.player.x, nextZ, PLAYER_RADIUS) && clearsGuards(state.player.x, nextZ)) {
      state.player.z = nextZ;
    }
  }

  function moveGuard(guard, dx, dz) {
    const nextX = guard.x + dx;
    const nextZ = guard.z + dz;
    const clearsActors = (x, z) =>
      Math.hypot(x - state.player.x, z - state.player.z) > PLAYER_RADIUS + 0.22 &&
      state.guards.every((other) => other === guard || other.dead || Math.hypot(x - other.x, z - other.z) > 0.4);
    if (isOpen(nextX, guard.z, 0.2) && clearsActors(nextX, guard.z)) guard.x = nextX;
    else guard.patrolAngle += Math.PI * 0.65;
    if (isOpen(guard.x, nextZ, 0.2) && clearsActors(guard.x, nextZ)) guard.z = nextZ;
    else guard.patrolAngle -= Math.PI * 0.42;
  }

  function getEffectMaterial(color) {
    if (!effectMaterials.has(color)) {
      effectMaterials.set(color, new THREE.MeshBasicMaterial({ color, transparent: true, opacity: 1 }));
    }
    return effectMaterials.get(color);
  }

  function spawnImpact(point, color = 0xffb34f, count = 7) {
    if (REDUCED_MOTION) count = Math.min(2, count);
    for (let i = 0; i < count && state.effects.length < 80; i += 1) {
      const mesh = new THREE.Mesh(sharedEffectGeometry, getEffectMaterial(color));
      mesh.position.copy(point);
      effectRoot.add(mesh);
      state.effects.push({
        mesh, life: 0.28 + Math.random() * 0.3,
        velocity: new THREE.Vector3((Math.random() - 0.5) * 2.4, Math.random() * 1.6, (Math.random() - 0.5) * 2.4),
      });
    }
  }

  function alertNearbyGuards(radius = 9) {
    for (const guard of state.guards) {
      if (guard.dead || Math.hypot(guard.x - state.player.x, guard.z - state.player.z) > radius) continue;
      guard.alert = Math.max(guard.alert, 4);
      guard.lastSeenX = state.player.x;
      guard.lastSeenZ = state.player.z;
      if (guard.ai === "patrol") guard.ai = "investigate";
    }
  }

  function fireWeapon() {
    const player = state.player;
    if (state.mode !== "playing" || player.cooldown > 0 || player.reload > 0) return;
    player.cooldown = state.aiming ? 0.2 : 0.18;
    state.recoil = 1;
    state.muzzleTimer = 0.055;
    ui.crosshair.classList.add("recoil");
    setTimeout(() => ui.crosshair.classList.remove("recoil"), 100);
    if (player.ammo <= 0) {
      audio.event("empty"); say("Mi serve un caricatore. Subito.", true); return;
    }
    player.ammo -= 1;
    audio.event("shot");
    alertNearbyGuards();
    updateCamera(FIXED_STEP);
    raycaster.setFromCamera(centerScreen, camera);
    raycaster.far = 18;
    const intersections = raycaster.intersectObjects(world.children, true);
    let resolved = false;
    for (const hit of intersections) {
      const guard = hit.object.userData.guard;
      if (guard && !guard.dead) {
        const headshot = hit.object.userData.hitZone === "head";
        guard.health -= headshot ? 2 : 1;
        guard.hit = 0.16;
        guard.stagger = 0.18;
        guard.alert = 5;
        guard.ai = "stagger";
        spawnImpact(hit.point, headshot ? 0xffe3a0 : 0xe85e4f, headshot ? 10 : 6);
        ui.hitMarker.classList.add("visible");
        ui.hitMarker.classList.toggle("fatal", guard.health <= 0);
        setTimeout(() => {
          ui.hitMarker.classList.remove("visible");
          ui.hitMarker.classList.remove("fatal");
        }, 120);
        audio.event("hit");
        if (guard.health <= 0) {
          guard.dead = true;
          guard.ai = "dead";
          state.score += headshot ? 160 : 100;
          audio.event("enemyDown");
          say(headshot ? "Preciso. Quasi elegante." : "Uno di meno. Che mira bestiale!", false);
        }
        resolved = true;
        break;
      }
      if (hit.object.userData.worldSolid) {
        spawnImpact(hit.point, 0xffc45c, 5);
        resolved = true;
        break;
      }
    }
    if (!resolved) {
      tempVector.set(0, 0, -8).applyQuaternion(camera.quaternion).add(camera.position);
      spawnImpact(tempVector, 0xffc45c, 3);
    }
  }

  function reloadWeapon() {
    const player = state.player;
    if (state.mode !== "playing" || player.reload > 0 || player.ammo === MAX_AMMO) return;
    player.reload = 0.95;
    audio.event("reload");
    say("Ricarico. Copritemi... anche se sono solo.", false);
  }

  function hurtPlayer(amount, sourceX = state.player.x, sourceZ = state.player.z) {
    if (state.mode !== "playing" || state.player.hurtCooldown > 0) return;
    state.player.hurtCooldown = 0.32;
    state.player.health = Math.max(0, state.player.health - amount);
    state.damageTimer = 0.24;
    const angle = normalizeAngle(Math.atan2(sourceZ - state.player.z, sourceX - state.player.x) - state.player.angle);
    ui.damageFlash.style.setProperty("--damage-angle", `${angle + Math.PI / 2}rad`);
    ui.damageFlash.classList.add("visible");
    setTimeout(() => ui.damageFlash.classList.remove("visible"), 170);
    audio.event("hurt");
    if (state.player.health <= 0) {
      state.mode = "gameOver";
      audio.event("gameOver"); audio.stopSpeech();
      showMainOverlay("MISSIONE FALLITA", "Agente a terra", "Mi arrendo... ma solo fino al prossimo tentativo.", "RIPROVA");
      say("Mi arrendo... ma solo per adesso.", true);
      releasePointer(); updateHud(true);
    }
  }

  function createEnemyShotView(direction) {
    const material = new THREE.MeshBasicMaterial({ color: 0xff7a35 });
    const mesh = new THREE.Mesh(new THREE.CylinderGeometry(0.022, 0.022, 0.42, 6), material);
    mesh.quaternion.setFromUnitVectors(yAxis, direction.clone().normalize());
    world.add(mesh);
    return mesh;
  }

  function fireEnemyRound(guard, distance) {
    if (state.enemyShots.length >= 24) return;
    const dx = state.player.x - guard.x;
    const dz = state.player.z - guard.z;
    const baseAngle = Math.atan2(dz, dx);
    const spread = (Math.random() - 0.5) * (0.075 + distance * 0.008);
    const angle = baseAngle + spread;
    const speed = 6.2 + state.levelIndex * 0.5;
    const direction = new THREE.Vector3(Math.cos(angle), 0, Math.sin(angle));
    const x = guard.x + direction.x * 0.45;
    const z = guard.z + direction.z * 0.45;
    const flightTime = Math.max(0.2, distance / speed);
    const shot = {
      x, z, y: 1.28, previousX: x, previousZ: z,
      vx: direction.x * speed, vz: direction.z * speed,
      vy: (PLAYER_HEIGHT - 1.28) / flightTime,
      life: 1.7, damage: 4 + Math.min(1, state.levelIndex), nearMiss: false,
    };
    shot.view = createEnemyShotView(new THREE.Vector3(shot.vx, shot.vy, shot.vz));
    shot.view.position.set(x, shot.y, z);
    state.enemyShots.push(shot);
    guard.muzzle = 0.1;
    audio.event("enemyShot");
  }

  function segmentDistanceToPlayer(shot) {
    const sx = shot.x - shot.previousX;
    const sz = shot.z - shot.previousZ;
    const lengthSquared = sx * sx + sz * sz;
    if (lengthSquared === 0) return Math.hypot(state.player.x - shot.x, state.player.z - shot.z);
    const t = Math.max(0, Math.min(1,
      ((state.player.x - shot.previousX) * sx + (state.player.z - shot.previousZ) * sz) / lengthSquared));
    return Math.hypot(state.player.x - (shot.previousX + sx * t), state.player.z - (shot.previousZ + sz * t));
  }

  function removeEnemyShot(index) {
    const shot = state.enemyShots[index];
    world.remove(shot.view);
    shot.view.geometry.dispose();
    shot.view.material.dispose();
    state.enemyShots.splice(index, 1);
  }

  function updateEnemyShots(dt) {
    for (let i = state.enemyShots.length - 1; i >= 0; i -= 1) {
      const shot = state.enemyShots[i];
      shot.previousX = shot.x; shot.previousZ = shot.z;
      shot.x += shot.vx * dt; shot.z += shot.vz * dt; shot.y += shot.vy * dt; shot.life -= dt;
      shot.view.position.set(shot.x, shot.y, shot.z);
      if (tileAt(shot.x, shot.z) !== 0 || hitsObstacle(shot.x, shot.z, 0.03, shot.y)) {
        spawnImpact(shot.view.position, 0xff6937, 5); removeEnemyShot(i); continue;
      }
      const proximity = segmentDistanceToPlayer(shot);
      if (!shot.nearMiss && proximity < 0.72 && proximity > PLAYER_RADIUS + 0.1) {
        shot.nearMiss = true; audio.event("nearMiss");
      }
      if (proximity < PLAYER_RADIUS + 0.08 && Math.abs(shot.y - PLAYER_HEIGHT) < 0.45) {
        const sourceX = shot.x - shot.vx * 0.1;
        const sourceZ = shot.z - shot.vz * 0.1;
        const damage = shot.damage;
        removeEnemyShot(i); hurtPlayer(damage, sourceX, sourceZ); continue;
      }
      if (shot.life <= 0) removeEnemyShot(i);
    }
  }

  function activeShooters() {
    return state.guards.filter((guard) => !guard.dead && ["telegraph", "burst", "melee"].includes(guard.ai)).length;
  }

  function setGuardEngaged(guard) {
    if (!["engage", "telegraph", "burst", "melee"].includes(guard.ai)) {
      audio.event("alert");
      guard.alertIcon.visible = true;
    }
    guard.ai = "engage";
    guard.alert = 5;
  }

  function updateGuardAI(guard, dt) {
    if (guard.dead) {
      guard.death = Math.min(1, guard.death + dt * 2.3);
      guard.view.rotation.z = -guard.death * Math.PI * 0.47;
      guard.view.position.y = -guard.death * 0.34;
      return;
    }
    guard.attack -= dt;
    guard.hit = Math.max(0, guard.hit - dt);
    guard.muzzle = Math.max(0, guard.muzzle - dt);
    guard.alert = Math.max(0, guard.alert - dt);
    guard.sway += dt * 5;
    guard.flash.visible = guard.muzzle > 0;
    guard.materials.forEach((material) => { material.emissiveIntensity = guard.hit > 0 ? 0.8 : 0; });

    const dx = state.player.x - guard.x;
    const dz = state.player.z - guard.z;
    const distance = Math.hypot(dx, dz);
    const playerBearing = Math.atan2(dz, dx);
    const facingAngle = guard.ai === "patrol"
      ? guard.patrolAngle
      : Math.atan2(guard.lastSeenZ - guard.z, guard.lastSeenX - guard.x);
    const inVisionCone = guard.alert > 0 || Math.abs(normalizeAngle(playerBearing - facingAngle)) < 0.92;
    const seesPlayer = distance < 7.5 && inVisionCone &&
      lineOfSight(guard.x, guard.z, state.player.x, state.player.z);
    if (seesPlayer) {
      guard.suspicion += dt;
      guard.lastSeenX = state.player.x;
      guard.lastSeenZ = state.player.z;
      if (guard.suspicion >= 0.72 - state.levelIndex * 0.1 &&
          ["patrol", "investigate", "engage"].includes(guard.ai)) {
        setGuardEngaged(guard);
      }
    } else {
      guard.suspicion = Math.max(0, guard.suspicion - dt * 0.45);
      if (guard.alert <= 0 && ["engage", "investigate"].includes(guard.ai)) guard.ai = "patrol";
    }

    if (guard.ai === "stagger") {
      guard.stagger -= dt;
      if (guard.stagger <= 0) guard.ai = "engage";
    } else if (guard.ai === "patrol") {
      guard.patrolTimer -= dt;
      if (guard.patrolTimer <= 0) {
        guard.patrolTimer = 1.2 + Math.random() * 2.4;
        guard.patrolAngle += (Math.random() - 0.5) * 2.2;
      }
      const speed = 0.32 * dt;
      moveGuard(guard, Math.cos(guard.patrolAngle) * speed, Math.sin(guard.patrolAngle) * speed);
    } else if (guard.ai === "investigate") {
      const ix = guard.lastSeenX - guard.x;
      const iz = guard.lastSeenZ - guard.z;
      const length = Math.hypot(ix, iz) || 1;
      const speed = 0.58 * dt;
      moveGuard(guard, ix / length * speed, iz / length * speed);
      if (length < 0.45 && guard.alert <= 0) guard.ai = "patrol";
    } else if (guard.ai === "engage") {
      if (!seesPlayer) {
        guard.ai = "investigate";
      } else {
        const nx = dx / Math.max(0.001, distance);
        const nz = dz / Math.max(0.001, distance);
        const speed = (0.55 + state.levelIndex * 0.04) * dt;
        if (distance > 4.6) moveGuard(guard, nx * speed, nz * speed);
        else if (distance < 2.1) moveGuard(guard, -nx * speed * 0.7, -nz * speed * 0.7);
        else moveGuard(guard, -nz * speed * 0.42 * guard.strafe, nx * speed * 0.42 * guard.strafe);
        if (distance < 0.86 && guard.attack <= 0) {
          guard.ai = "melee"; guard.telegraph = 0.34; guard.alertIcon.visible = true;
        } else if (guard.attack <= 0 && activeShooters() < (state.levelIndex === 0 ? 1 : 2)) {
          guard.ai = "telegraph"; guard.telegraph = 0.22; guard.alertIcon.visible = true;
        }
      }
    } else if (guard.ai === "telegraph") {
      guard.telegraph -= dt;
      if (guard.telegraph <= 0) {
        guard.ai = "burst";
        guard.burstRemaining = state.levelIndex === 2 ? 3 : 2;
        guard.burstGap = 0;
      }
    } else if (guard.ai === "burst") {
      guard.burstGap -= dt;
      if (guard.burstGap <= 0 && guard.burstRemaining > 0) {
        fireEnemyRound(guard, distance);
        guard.burstRemaining -= 1;
        guard.burstGap = 0.13;
      }
      if (guard.burstRemaining <= 0) {
        guard.ai = "engage";
        guard.attack = 1.2 - state.levelIndex * 0.1 + Math.random() * 0.25;
        guard.alertIcon.visible = false;
      }
    } else if (guard.ai === "melee") {
      guard.telegraph -= dt;
      if (guard.telegraph <= 0) {
        if (distance < 1.02) {
          audio.event("melee");
          hurtPlayer(11 + state.levelIndex, guard.x, guard.z);
        }
        guard.ai = "engage"; guard.attack = 0.8; guard.alertIcon.visible = false;
      }
    }

    const facing = Math.atan2(state.player.z - guard.z, state.player.x - guard.x);
    const visualAngle = ["engage", "telegraph", "burst", "melee", "stagger"].includes(guard.ai) ? facing : guard.patrolAngle;
    guard.view.rotation.y = Math.PI / 2 - visualAngle;
    guard.view.position.x = guard.x;
    guard.view.position.z = guard.z;
    guard.alertIcon.visible = guard.ai === "telegraph" || guard.ai === "melee" || (guard.suspicion > 0.15 && guard.ai === "patrol");
    guard.alertIcon.rotation.y += dt * 3;
    const gait = Math.sin(guard.sway) * (guard.ai === "patrol" || guard.ai === "investigate" || guard.ai === "engage" ? 0.28 : 0.05);
    if (guard.legs.length === 2) {
      guard.legs[0].rotation.x = gait;
      guard.legs[1].rotation.x = -gait;
    }
  }

  function updatePickups(dt) {
    for (let i = state.pickups.length - 1; i >= 0; i -= 1) {
      const pickup = state.pickups[i];
      pickup.phase += dt * 2;
      pickup.view.rotation.y += dt * 1.4;
      pickup.view.position.y = 0.58 + Math.sin(pickup.phase) * 0.08;
      const distance = Math.hypot(pickup.x - state.player.x, pickup.z - state.player.z);
      if (pickup.type === "intel" && distance < 3.2) audio.event("objectiveAmbience");
      if (distance > 0.56) continue;
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
      world.remove(pickup.view);
      disposeObjectResources(pickup.view);
      state.pickups.splice(i, 1);
    }
  }

  function updateEffects(dt) {
    for (let i = state.effects.length - 1; i >= 0; i -= 1) {
      const effect = state.effects[i];
      effect.life -= dt;
      effect.velocity.y -= dt * 3.2;
      effect.mesh.position.addScaledVector(effect.velocity, dt);
      effect.mesh.scale.setScalar(Math.max(0.1, effect.life * 2));
      if (effect.life <= 0) {
        effectRoot.remove(effect.mesh);
        state.effects.splice(i, 1);
      }
    }
  }

  function updateCamera(dt) {
    const player = state.player;
    const bob = !REDUCED_MOTION && player.moving ? Math.sin(player.bob) * 0.025 : 0;
    const shake = !REDUCED_MOTION && state.damageTimer > 0 ? state.damageTimer * 0.08 : 0;
    camera.position.set(
      player.x + (Math.random() - 0.5) * shake,
      PLAYER_HEIGHT + bob + (Math.random() - 0.5) * shake,
      player.z + (Math.random() - 0.5) * shake,
    );
    tempVector.set(
      player.x + Math.cos(player.angle) * 5,
      PLAYER_HEIGHT + Math.tan(player.pitch) * 5,
      player.z + Math.sin(player.angle) * 5,
    );
    camera.lookAt(tempVector);
    const targetFov = state.aiming ? 56 : FOV;
    camera.fov += (targetFov - camera.fov) * Math.min(1, dt * 10);
    camera.updateProjectionMatrix();
    const recoil = state.recoil * 0.07;
    const reloadDip = player.reload > 0 ? Math.sin((player.reload / 0.95) * Math.PI) * 0.28 : 0;
    const targetX = state.aiming ? 0 : 0.3;
    const targetY = state.aiming ? -0.23 : -0.33;
    weaponView.position.x += (targetX - weaponView.position.x) * Math.min(1, dt * 12);
    weaponView.position.y += (targetY - reloadDip - weaponView.position.y) * Math.min(1, dt * 12);
    weaponView.position.z = -0.58 + recoil;
    weaponView.rotation.z = player.moving && !REDUCED_MOTION ? Math.sin(player.bob * 0.5) * 0.018 : 0;
    weaponFlash.visible = state.muzzleTimer > 0;
    weaponFlash.rotation.z += dt * 24;
    weaponLight.intensity = state.muzzleTimer > 0 ? 8 : 0;
  }

  function update(dt) {
    if (state.mode !== "playing") return;
    const player = state.player;
    state.elapsed += dt;
    player.cooldown = Math.max(0, player.cooldown - dt);
    player.hurtCooldown = Math.max(0, player.hurtCooldown - dt);
    state.damageTimer = Math.max(0, state.damageTimer - dt);
    state.muzzleTimer = Math.max(0, state.muzzleTimer - dt);
    state.recoil = Math.max(0, state.recoil - dt * 6.5);
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
    player.sprinting = player.moving && input.keys.has("ShiftLeft") && forward > 0.2 && !state.aiming;
    if (magnitude > 0) {
      forward /= Math.max(1, magnitude);
      strafe /= Math.max(1, magnitude);
      const speed = (player.sprinting ? 3.5 : state.aiming ? 1.65 : 2.45) * dt;
      const dx = (Math.cos(player.angle) * forward + Math.cos(player.angle + Math.PI / 2) * strafe) * speed;
      const dz = (Math.sin(player.angle) * forward + Math.sin(player.angle + Math.PI / 2) * strafe) * speed;
      movePlayer(dx, dz);
      player.bob += dt * (player.sprinting ? 12 : 8.5);
      player.stepTimer -= dt;
      if (player.stepTimer <= 0) {
        audio.event("footstep", { weight: player.sprinting ? 1.12 : 0.78 });
        player.stepTimer = player.sprinting ? 0.28 : 0.4;
      }
    } else {
      player.stepTimer = 0;
    }
    if (input.fire) fireWeapon();
    updatePickups(dt);
    state.guards.forEach((guard) => updateGuardAI(guard, dt));
    updateEnemyShots(dt);
    updateEffects(dt);
    state.dialogueTimer -= dt;
    state.randomDialogueTimer -= dt;
    if (state.dialogueTimer <= 0) ui.dialogue.classList.add("is-hidden");
    if (state.randomDialogueTimer <= 0) {
      state.randomDialogueTimer = 11 + Math.random() * 10;
      say(dialoguePool[Math.floor(Math.random() * dialoguePool.length)], false);
    }
    if (state.guards.every((guard) => guard.dead) && state.intel && !state.extraction) {
      activateExtraction();
    }
    if (state.extraction) {
      state.extractionView.userData.ring.rotation.z += dt * 0.7;
      const pulse = 1 + Math.sin(state.elapsed * 4) * 0.08;
      state.extractionView.scale.setScalar(pulse);
      const spawn = LEVELS[state.levelIndex].spawn;
      if (Math.hypot(state.player.x - spawn[0], state.player.z - spawn[1]) < 0.72) completeLevel();
    }
    updateCamera(dt);
    updateHud();
  }

  function activateExtraction() {
    state.extraction = true;
    const spawn = LEVELS[state.levelIndex].spawn;
    const marker = new THREE.Group();
    const ring = new THREE.Mesh(
      new THREE.TorusGeometry(0.52, 0.055, 8, 32),
      new THREE.MeshBasicMaterial({ color: 0x64f5c8 }),
    );
    ring.rotation.x = Math.PI / 2;
    marker.add(ring);
    marker.userData.ring = ring;
    const beacon = new THREE.Mesh(
      new THREE.CylinderGeometry(0.28, 0.5, 1.8, 20, 1, true),
      new THREE.MeshBasicMaterial({
        color: 0x64f5c8, transparent: true, opacity: 0.12,
        blending: THREE.AdditiveBlending, depthWrite: false,
      }),
    );
    beacon.position.y = 0.9;
    marker.add(beacon);
    if (!MOBILE) {
      const light = new THREE.PointLight(0x64f5c8, 2.6, 4, 2);
      light.position.y = 0.5;
      marker.add(light);
    }
    marker.position.set(spawn[0], 0.08, spawn[1]);
    state.extractionView = marker;
    world.add(marker);
    ui.objective.textContent = "Raggiungi il punto di estrazione.";
    audio.event("objectiveAmbience");
    say("Obiettivo completato. Torno al punto di estrazione.", true);
    announce("Punto di estrazione attivo.");
  }

  function armTestExtraction() {
    state.guards.forEach((guard) => { guard.dead = true; guard.ai = "dead"; });
    state.intel = true;
    const spawn = LEVELS[state.levelIndex].spawn;
    const staging = openCells().find((cell) =>
      Math.hypot(cell.x - spawn[0], cell.z - spawn[1]) > 0.9 &&
      Math.hypot(cell.x - spawn[0], cell.z - spawn[1]) < 2.2 &&
      !hitsObstacle(cell.x, cell.z, PLAYER_RADIUS));
    if (staging) {
      state.player.x = staging.x;
      state.player.z = staging.z;
    }
    if (!state.extraction) activateExtraction();
    updateHud(true);
  }

  function completeLevel() {
    if (state.mode !== "playing") return;
    state.mode = "levelComplete"; audio.event("levelClear"); releasePointer();
    if (state.levelIndex < LEVELS.length - 1) {
      showMainOverlay("MISSIONE COMPIUTA", LEVELS[state.levelIndex].name,
        "Documenti al sicuro. La prossima operazione è già iniziata.", "PROSSIMA MISSIONE");
      say("Missione compiuta. Non avevo dubbi.", true);
    } else {
      state.mode = "victory"; audio.event("victory");
      showMainOverlay("OPERAZIONE CONCLUSA", "Vittoria gorillesca",
        `Punteggio finale: ${state.score}. La giungla canta in italiano.`, "GIOCA ANCORA");
      say("Missione compiuta. La giungla canta in italiano!", true);
    }
    updateHud(true);
  }

  function advanceLevel() {
    if (state.mode !== "levelComplete") return;
    loadLevel(state.levelIndex + 1, true); state.mode = "playing"; hideMainOverlay();
    audio.setPaused(false); updateHud(true); requestPointerLock();
  }

  function updateHud(force = false) {
    if (!state.player) return;
    const alive = state.guards.filter((guard) => !guard.dead).length;
    const snapshot = `${state.mode}|${state.player.health}|${state.player.ammo}|${state.player.reload > 0}|${alive}|${state.intel}|${state.extraction}|${state.levelIndex}|${state.enemyShots.length}`;
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
    document.body.dataset.extraction = String(state.extraction);
    document.body.dataset.enemyShots = String(state.enemyShots.length);
    document.body.dataset.engine = "threejs-webgl";
  }

  function resize() {
    if (!renderer || !camera) return;
    const pixelRatio = Math.min(devicePixelRatio || 1, MOBILE ? 1.25 : 1.75) * qualityScale;
    renderer.setPixelRatio(pixelRatio);
    renderer.setSize(innerWidth, innerHeight, false);
    camera.aspect = innerWidth / Math.max(1, innerHeight);
    camera.updateProjectionMatrix();
    viewmodelCamera.aspect = camera.aspect;
    viewmodelCamera.updateProjectionMatrix();
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
    if (state.mode === "levelComplete") advanceLevel(); else startGame();
  }

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

  window.addEventListener("keydown", (event) => {
    if (["Space", "ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"].includes(event.code)) event.preventDefault();
    if (event.repeat && ["KeyR", "KeyP", "KeyM", "Escape"].includes(event.code)) return;
    if (event.code === "KeyP") { state.mode === "paused" ? resumeGame() : pauseGame(); return; }
    if (event.code === "Escape" && state.mode === "paused") { resumeGame(); return; }
    if (event.code === "KeyM") { toggleMute(); return; }
    if (TEST_MODE && event.code === "KeyN") {
      if (!state.extraction) armTestExtraction();
      else {
        const spawn = LEVELS[state.levelIndex].spawn;
        state.player.x = spawn[0]; state.player.z = spawn[1]; completeLevel();
      }
      return;
    }
    if (TEST_MODE && event.code === "KeyG" && state.mode === "playing") {
      const guard = state.guards.find((candidate) => !candidate.dead);
      if (guard) {
        guard.x = state.player.x + Math.cos(state.player.angle) * 3;
        guard.z = state.player.z + Math.sin(state.player.angle) * 3;
        guard.suspicion = 2; guard.attack = 0; setGuardEngaged(guard);
      }
      return;
    }
    if (event.code === "KeyR") {
      if (state.mode === "gameOver" || state.mode === "victory") restartGame(); else reloadWeapon();
      return;
    }
    if (event.code === "Space") {
      if (state.mode === "ready") startGame();
      else if (state.mode === "playing") { input.fire = true; fireWeapon(); }
    }
    input.keys.add(event.code);
  });

  window.addEventListener("keyup", (event) => {
    input.keys.delete(event.code);
    if (event.code === "Space") input.fire = false;
  });
  window.addEventListener("blur", () => { if (state.mode === "playing") pauseGame(); clearInputs(); });
  document.addEventListener("visibilitychange", () => { if (document.hidden && state.mode === "playing") pauseGame(); });
  document.addEventListener("pointerlockchange", () => {
    if (suppressPointerPause) { suppressPointerPause = false; return; }
    if (document.pointerLockElement !== canvas && state.mode === "playing" && matchMedia("(pointer: fine)").matches) pauseGame(true);
  });
  document.addEventListener("mousemove", (event) => {
    if (document.pointerLockElement !== canvas || state.mode !== "playing") return;
    state.player.angle += event.movementX * 0.00235;
    state.player.pitch = Math.max(-0.72, Math.min(0.72, state.player.pitch - event.movementY * 0.0019));
  });
  canvas.addEventListener("mousedown", (event) => {
    audio.init();
    if (event.button === 2 && state.mode === "playing") { state.aiming = true; return; }
    if (event.button !== 0) return;
    if (state.mode === "ready") startGame();
    else if (state.mode === "playing") {
      if (document.pointerLockElement !== canvas) requestPointerLock();
      input.fire = true; fireWeapon();
    }
  });
  window.addEventListener("mouseup", (event) => {
    if (event.button === 0) input.fire = false;
    if (event.button === 2) state.aiming = false;
  });
  canvas.addEventListener("contextmenu", (event) => event.preventDefault());
  ui.startButton.addEventListener("click", handleStartButton);
  ui.pauseButton.addEventListener("click", () => state.mode === "paused" ? resumeGame() : pauseGame());
  ui.resumeButton.addEventListener("click", resumeGame);
  ui.restartButton.addEventListener("click", restartGame);
  ui.muteButton.addEventListener("click", toggleMute);
  ui.touchReload.addEventListener("click", reloadWeapon);
  ui.movePad.addEventListener("pointerdown", (event) => {
    audio.init(); input.movePointer = event.pointerId; ui.movePad.setPointerCapture(event.pointerId); updateMovePad(event);
  });
  ui.movePad.addEventListener("pointermove", (event) => { if (event.pointerId === input.movePointer) updateMovePad(event); });
  const endMove = (event) => {
    if (event.pointerId !== input.movePointer) return;
    input.movePointer = null; input.moveX = 0; input.moveY = 0;
    ui.moveStick.style.transform = "translate(-50%, -50%)";
  };
  ui.movePad.addEventListener("pointerup", endMove);
  ui.movePad.addEventListener("pointercancel", endMove);
  ui.movePad.addEventListener("lostpointercapture", endMove);
  ui.lookZone.addEventListener("pointerdown", (event) => {
    audio.init(); input.lookPointer = event.pointerId; input.lookX = event.clientX; input.lookY = event.clientY;
    ui.lookZone.setPointerCapture(event.pointerId);
  });
  ui.lookZone.addEventListener("pointermove", (event) => {
    if (event.pointerId !== input.lookPointer || state.mode !== "playing") return;
    state.player.angle += (event.clientX - input.lookX) * 0.008;
    state.player.pitch = Math.max(-0.72, Math.min(0.72, state.player.pitch - (event.clientY - input.lookY) * 0.005));
    input.lookX = event.clientX; input.lookY = event.clientY;
  });
  const endLook = (event) => { if (event.pointerId === input.lookPointer) input.lookPointer = null; };
  ui.lookZone.addEventListener("pointerup", endLook);
  ui.lookZone.addEventListener("pointercancel", endLook);
  ui.lookZone.addEventListener("lostpointercapture", endLook);
  ui.touchFire.addEventListener("pointerdown", (event) => {
    event.preventDefault(); audio.init(); input.fire = true; fireWeapon(); ui.touchFire.setPointerCapture(event.pointerId);
  });
  ui.touchFire.addEventListener("pointerup", () => { input.fire = false; });
  ui.touchFire.addEventListener("pointercancel", () => { input.fire = false; });
  ui.touchFire.addEventListener("lostpointercapture", () => { input.fire = false; });
  window.addEventListener("resize", resize);

  function loop(now) {
    const delta = Math.min(0.1, (now - lastTime) / 1000);
    lastTime = now;
    if (state.mode === "playing") {
      accumulator += delta;
      let steps = 0;
      while (accumulator >= FIXED_STEP && steps < MAX_STEPS) {
        update(FIXED_STEP); accumulator -= FIXED_STEP; steps += 1;
      }
      if (steps === MAX_STEPS && accumulator >= FIXED_STEP) accumulator %= FIXED_STEP;
      if (delta > 0.029) slowFrames += 1; else slowFrames = Math.max(0, slowFrames - 2);
      if (slowFrames > 90 && qualityScale > 0.62) {
        qualityScale = Math.max(0.62, qualityScale - 0.12); slowFrames = 0; resize();
      }
    } else {
      accumulator = 0;
      updateCamera(Math.min(delta, 0.05));
    }
    renderer.autoClear = true;
    renderer.render(scene, camera);
    renderer.autoClear = false;
    renderer.clearDepth();
    renderer.render(viewmodelScene, viewmodelCamera);
    requestAnimationFrame(loop);
  }

  if (!initializeRenderer()) return;
  if (TEST_MODE) {
    window.__GORILLA_TEST__ = {
      getState: () => ({
        mode: state.mode, levelIndex: state.levelIndex, health: state.player.health,
        ammo: state.player.ammo, x: state.player.x, z: state.player.z,
        guardsAlive: state.guards.filter((guard) => !guard.dead).length,
        enemyShots: state.enemyShots.length, intel: state.intel, score: state.score,
        engine: "threejs-webgl",
      }),
      start: startGame,
      damagePlayer: hurtPlayer,
      completeObjective: () => {
        armTestExtraction();
      },
      finishExtraction: () => {
        const spawn = LEVELS[state.levelIndex].spawn;
        state.player.x = spawn[0]; state.player.z = spawn[1]; completeLevel();
      },
      spawnAttacker: () => {
        const guard = state.guards.find((candidate) => !candidate.dead);
        if (!guard) return false;
        guard.x = state.player.x + Math.cos(state.player.angle) * 3;
        guard.z = state.player.z + Math.sin(state.player.angle) * 3;
        guard.suspicion = 2; guard.attack = 0; setGuardEngaged(guard); return true;
      },
      advanceLevel, restart: restartGame,
    };
  }
  loadLevel(0, false);
  ui.muteButton.setAttribute("aria-pressed", String(audio.isMuted()));
  requestAnimationFrame(loop);
})();
