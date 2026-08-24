(function () {
  "use strict";

  const STORAGE_KEY = "gorillaGoldenMute";
  const MAX_VOICES = 30;
  const SILENCE = 0.0001;
  const MASTER_LEVEL = 0.68;

  let context = null;
  let masterGain = null;
  let effectsGain = null;
  let reverbInput = null;
  let reverbGain = null;
  let noiseBuffer = null;
  let initialized = false;
  let unavailable = false;
  let paused = false;
  let muted = readStoredMute();
  let speechUtterance = null;

  const voices = [];
  const lastEventAt = Object.create(null);

  function safely(action, fallback) {
    try {
      return action();
    } catch (_error) {
      return fallback;
    }
  }

  function readStoredMute() {
    return safely(function () {
      return window.localStorage.getItem(STORAGE_KEY) === "true";
    }, false);
  }

  function storeMute() {
    safely(function () {
      window.localStorage.setItem(STORAGE_KEY, String(muted));
    });
  }

  function hasUserGesture() {
    return safely(function () {
      const activation = window.navigator && window.navigator.userActivation;
      return !activation || activation.isActive || activation.hasBeenActive;
    }, true);
  }

  function audioNow() {
    return context ? context.currentTime : 0;
  }

  function setMasterLevel(rampSeconds) {
    if (!initialized || !masterGain || !context) return;

    safely(function () {
      const time = audioNow();
      const target = muted || paused ? SILENCE : MASTER_LEVEL;
      masterGain.gain.cancelScheduledValues(time);
      masterGain.gain.setTargetAtTime(target, time, rampSeconds || 0.025);
    });
  }

  function createNoiseBuffer() {
    const length = Math.max(1, Math.floor(context.sampleRate));
    const buffer = context.createBuffer(1, length, context.sampleRate);
    const samples = buffer.getChannelData(0);
    let previous = 0;

    for (let index = 0; index < samples.length; index += 1) {
      const white = Math.random() * 2 - 1;
      previous = previous * 0.58 + white * 0.42;
      samples[index] = white * 0.72 + previous * 0.28;
    }

    return buffer;
  }

  function createImpulseResponse() {
    const duration = 1.15;
    const length = Math.max(1, Math.floor(context.sampleRate * duration));
    const impulse = context.createBuffer(2, length, context.sampleRate);

    for (let channel = 0; channel < impulse.numberOfChannels; channel += 1) {
      const samples = impulse.getChannelData(channel);
      let filtered = 0;

      for (let index = 0; index < samples.length; index += 1) {
        const progress = index / samples.length;
        const white = Math.random() * 2 - 1;
        filtered = filtered * 0.32 + white * 0.68;
        samples[index] = filtered * Math.pow(1 - progress, 3.8) * (channel ? 0.82 : 0.9);
      }
    }

    return impulse;
  }

  function buildAudioGraph() {
    const compressor = context.createDynamicsCompressor();
    const convolver = context.createConvolver();
    masterGain = context.createGain();
    effectsGain = context.createGain();
    reverbInput = context.createGain();
    reverbGain = context.createGain();

    compressor.threshold.value = -12;
    compressor.knee.value = 8;
    compressor.ratio.value = 6;
    compressor.attack.value = 0.002;
    compressor.release.value = 0.14;
    masterGain.gain.value = muted || paused ? SILENCE : MASTER_LEVEL;
    effectsGain.gain.value = 0.82;
    reverbInput.gain.value = 0.22;
    reverbGain.gain.value = 0.16;
    convolver.buffer = createImpulseResponse();

    effectsGain.connect(masterGain);
    effectsGain.connect(reverbInput);
    reverbInput.connect(convolver);
    convolver.connect(reverbGain);
    reverbGain.connect(masterGain);
    masterGain.connect(compressor);
    compressor.connect(context.destination);
    noiseBuffer = createNoiseBuffer();
  }

  function init() {
    if (initialized) {
      if (hasUserGesture() && context && context.state === "suspended") {
        safely(function () {
          const resumeResult = context.resume();
          if (resumeResult && typeof resumeResult.catch === "function") {
            resumeResult.catch(function () {});
          }
        });
      }
      return true;
    }

    if (unavailable || !hasUserGesture()) return false;

    return safely(function () {
      const AudioContextClass = window.AudioContext || window.webkitAudioContext;
      if (!AudioContextClass) {
        unavailable = true;
        return false;
      }

      context = new AudioContextClass();
      buildAudioGraph();
      initialized = true;

      if (context.state === "suspended") {
        const resumeResult = context.resume();
        if (resumeResult && typeof resumeResult.catch === "function") {
          resumeResult.catch(function () {});
        }
      }

      setMasterLevel(0.02);
      return true;
    }, false);
  }

  function removeVoice(voice) {
    const index = voices.indexOf(voice);
    if (index !== -1) voices.splice(index, 1);
    safely(function () {
      voice.output.disconnect();
    });
  }

  function stopVoice(voice) {
    if (!voice || voice.stopping) return;
    voice.stopping = true;

    safely(function () {
      const time = audioNow();
      voice.output.gain.cancelScheduledValues(time);
      voice.output.gain.setTargetAtTime(SILENCE, time, 0.006);
      voice.sources.forEach(function (source) {
        safely(function () {
          source.stop(time + 0.025);
        });
      });
    });
  }

  function registerVoice(sources, output, stopAt) {
    while (voices.length >= MAX_VOICES) stopVoice(voices.shift());

    const voice = {
      sources: sources,
      output: output,
      remaining: sources.length,
      stopping: false
    };
    voices.push(voice);

    if (!sources.length) {
      removeVoice(voice);
      return false;
    }

    sources.forEach(function (source) {
      source.addEventListener("ended", function () {
        voice.remaining -= 1;
        if (voice.remaining <= 0) removeVoice(voice);
      }, { once: true });

      safely(function () {
        source.start(audioNow());
        source.stop(stopAt);
      });
    });

    return true;
  }

  function scheduleEnvelope(param, time, attack, duration, level) {
    param.setValueAtTime(SILENCE, time);
    param.exponentialRampToValueAtTime(Math.max(SILENCE, level), time + attack);
    param.exponentialRampToValueAtTime(SILENCE, time + duration);
  }

  function randomBetween(minimum, maximum) {
    return minimum + Math.random() * (maximum - minimum);
  }

  function clamp(value, minimum, maximum) {
    return Math.max(minimum, Math.min(maximum, value));
  }

  function addTone(output, startTime, options) {
    const oscillator = context.createOscillator();
    const gain = context.createGain();
    const duration = Math.max(0.015, options.duration || 0.1);
    const startFrequency = Math.max(20, options.frequency || 220);
    const endFrequency = Math.max(20, options.endFrequency || startFrequency);

    oscillator.type = options.type || "triangle";
    gain.gain.value = SILENCE;
    oscillator.frequency.setValueAtTime(startFrequency, startTime);
    oscillator.frequency.exponentialRampToValueAtTime(endFrequency, startTime + duration);
    if (options.detune) oscillator.detune.value = options.detune;
    scheduleEnvelope(gain.gain, startTime, options.attack || 0.004, duration, options.gain || 0.08);
    oscillator.connect(gain).connect(output);
    return oscillator;
  }

  function addNoise(output, startTime, options) {
    const source = context.createBufferSource();
    const filter = context.createBiquadFilter();
    const gain = context.createGain();
    const duration = Math.max(0.015, options.duration || 0.1);

    source.buffer = noiseBuffer;
    source.loop = true;
    source.playbackRate.value = options.rate || 1;
    gain.gain.value = SILENCE;
    filter.type = options.filterType || "bandpass";
    filter.frequency.setValueAtTime(options.frequency || 1200, startTime);
    if (options.endFrequency) {
      filter.frequency.exponentialRampToValueAtTime(
        Math.max(20, options.endFrequency),
        startTime + duration
      );
    }
    filter.Q.value = options.q || 0.8;
    scheduleEnvelope(gain.gain, startTime, options.attack || 0.003, duration, options.gain || 0.1);
    source.connect(filter).connect(gain).connect(output);
    return source;
  }

  function playVoice(duration, build, details) {
    if (!initialized || !context || !effectsGain || muted || paused) return false;

    return safely(function () {
      if (context.state === "suspended" && hasUserGesture()) {
        const resumeResult = context.resume();
        if (resumeResult && typeof resumeResult.catch === "function") {
          resumeResult.catch(function () {});
        }
      }

      const startTime = audioNow();
      const output = context.createGain();
      const destination = typeof context.createStereoPanner === "function"
        ? context.createStereoPanner()
        : context.createGain();
      output.gain.value = 1;
      if (destination.pan) {
        destination.pan.value = clamp(Number(details && details.pan) || 0, -1, 1);
      }
      output.connect(destination).connect(effectsGain);
      const sources = build(output, startTime).filter(Boolean);
      return registerVoice(sources, output, startTime + duration + 0.035);
    }, false);
  }

  function playShot(details) {
    const pitch = randomBetween(0.96, 1.04);
    return playVoice(0.38, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.035, gain: 0.31, frequency: 5600, filterType: "highpass", q: 0.55 }),
        addNoise(output, time, { duration: 0.15, gain: 0.3, frequency: 1100 * pitch, endFrequency: 430, q: 0.65 }),
        addTone(output, time, { type: "sawtooth", duration: 0.2, gain: 0.16, frequency: 168 * pitch, endFrequency: 43 }),
        addNoise(output, time + 0.07, { duration: 0.27, gain: 0.105, frequency: 2100, endFrequency: 520, filterType: "lowpass", q: 0.5 }),
        addTone(output, time + 0.065, { type: "square", duration: 0.045, gain: 0.035, frequency: 2100, endFrequency: 980 })
      ];
    }, details);
  }

  function playEnemyShot(details) {
    const pitch = randomBetween(0.92, 1.08);
    return playVoice(0.32, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.028, gain: 0.23, frequency: 6400, filterType: "highpass", q: 0.7 }),
        addNoise(output, time, { duration: 0.12, gain: 0.22, frequency: 1450 * pitch, endFrequency: 510, q: 0.82 }),
        addTone(output, time, { type: "square", duration: 0.17, gain: 0.095, frequency: 238 * pitch, endFrequency: 62 }),
        addNoise(output, time + 0.06, { duration: 0.21, gain: 0.07, frequency: 2400, endFrequency: 680, filterType: "lowpass", q: 0.6 })
      ];
    }, details);
  }

  function playMelee(details) {
    return playVoice(0.42, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.17, gain: 0.12, frequency: 420, endFrequency: 2600, q: 0.7 }),
        addNoise(output, time + 0.115, { duration: 0.19, gain: 0.27, frequency: 760, endFrequency: 250, filterType: "lowpass", q: 0.55 }),
        addTone(output, time + 0.12, { type: "triangle", duration: 0.23, gain: 0.15, frequency: 112, endFrequency: 38 }),
        addNoise(output, time + 0.145, { duration: 0.055, gain: 0.085, frequency: 3300, q: 1.8 })
      ];
    }, details);
  }

  function playEmpty(details) {
    return playVoice(0.16, function (output, time) {
      return [
        addTone(output, time, { type: "square", duration: 0.042, gain: 0.062, frequency: 1320, endFrequency: 710 }),
        addNoise(output, time, { duration: 0.028, gain: 0.05, frequency: 4600, filterType: "highpass", q: 1.5 }),
        addTone(output, time + 0.07, { type: "triangle", duration: 0.05, gain: 0.04, frequency: 680, endFrequency: 430 })
      ];
    }, details);
  }

  function playReload(details) {
    return playVoice(0.82, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.065, gain: 0.095, frequency: 4300, q: 2.5 }),
        addTone(output, time + 0.12, { type: "square", duration: 0.055, gain: 0.052, frequency: 610, endFrequency: 920 }),
        addNoise(output, time + 0.3, { duration: 0.095, gain: 0.12, frequency: 3100, endFrequency: 1500, q: 2.3 }),
        addTone(output, time + 0.43, { type: "triangle", duration: 0.08, gain: 0.068, frequency: 760, endFrequency: 470 }),
        addNoise(output, time + 0.62, { duration: 0.07, gain: 0.14, frequency: 2500, q: 2.7 }),
        addTone(output, time + 0.63, { type: "square", duration: 0.085, gain: 0.055, frequency: 430, endFrequency: 260 })
      ];
    }, details);
  }

  function playHit(details) {
    const ring = randomBetween(920, 1380);
    return playVoice(0.3, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.032, gain: 0.18, frequency: 5200, filterType: "highpass", q: 0.75 }),
        addNoise(output, time, { duration: 0.15, gain: 0.18, frequency: 1050, endFrequency: 280, q: 1.1 }),
        addTone(output, time + 0.008, { type: "sine", duration: 0.17, gain: 0.055, frequency: ring, endFrequency: ring * 0.55 }),
        addTone(output, time, { type: "triangle", duration: 0.18, gain: 0.075, frequency: 150, endFrequency: 55 })
      ];
    }, details);
  }

  function playEnemyDown(details) {
    return playVoice(0.58, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.12, gain: 0.1, frequency: 1400, endFrequency: 480, q: 0.65 }),
        addTone(output, time, { type: "sawtooth", duration: 0.48, gain: 0.08, frequency: 205, endFrequency: 46 }),
        addNoise(output, time + 0.12, { duration: 0.38, gain: 0.135, frequency: 680, endFrequency: 190, filterType: "lowpass", q: 0.62 }),
        addTone(output, time + 0.18, { type: "triangle", duration: 0.28, gain: 0.07, frequency: 88, endFrequency: 39 })
      ];
    }, details);
  }

  function playHurt(details) {
    return playVoice(0.48, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.055, gain: 0.2, frequency: 2100, filterType: "highpass", q: 0.6 }),
        addNoise(output, time, { duration: 0.37, gain: 0.21, frequency: 620, endFrequency: 180, filterType: "lowpass", q: 0.62 }),
        addTone(output, time, { type: "sawtooth", duration: 0.34, gain: 0.105, frequency: 116, endFrequency: 49 }),
        addTone(output, time + 0.06, { type: "sine", duration: 0.32, gain: 0.035, frequency: 62, endFrequency: 42 })
      ];
    }, details);
  }

  function playPickup(details) {
    const root = details && details.kind === "health" ? 440 : 523.25;
    return playVoice(0.55, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.055, gain: 0.05, frequency: 4800, filterType: "highpass", q: 1.2 }),
        addTone(output, time, { type: "sine", duration: 0.2, gain: 0.075, frequency: root, endFrequency: root * 1.25 }),
        addTone(output, time + 0.1, { type: "triangle", duration: 0.22, gain: 0.07, frequency: root * 1.25, endFrequency: root * 1.5 }),
        addTone(output, time + 0.22, { type: "sine", duration: 0.27, gain: 0.085, frequency: root * 1.5, endFrequency: root * 2 }),
        addTone(output, time + 0.235, { type: "sine", duration: 0.24, gain: 0.025, frequency: root * 3, endFrequency: root * 2.5 })
      ];
    }, details);
  }

  function playLevelClear(details) {
    return playVoice(1.32, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.08, gain: 0.055, frequency: 5000, filterType: "highpass", q: 0.8 }),
        addTone(output, time, { type: "triangle", duration: 0.3, gain: 0.08, frequency: 196, endFrequency: 392 }),
        addTone(output, time + 0.18, { type: "sine", duration: 0.34, gain: 0.09, frequency: 392, endFrequency: 523.25 }),
        addTone(output, time + 0.42, { type: "triangle", duration: 0.38, gain: 0.095, frequency: 523.25, endFrequency: 659.25 }),
        addTone(output, time + 0.68, { type: "sine", duration: 0.58, gain: 0.105, frequency: 659.25, endFrequency: 1046.5 }),
        addTone(output, time + 0.69, { type: "sine", duration: 0.55, gain: 0.04, frequency: 329.63, endFrequency: 523.25 })
      ];
    }, details);
  }

  function playVictory(details) {
    return playVoice(2.24, function (output, time) {
      const notes = [261.63, 329.63, 392, 523.25, 659.25, 783.99];
      const sources = [];
      notes.forEach(function (frequency, index) {
        sources.push(addTone(output, time + index * 0.2, {
          type: index < 3 ? "triangle" : "sine",
          duration: index === notes.length - 1 ? 1.05 : 0.32,
          gain: index === notes.length - 1 ? 0.115 : 0.072,
          frequency: frequency,
          endFrequency: index === notes.length - 1 ? 1046.5 : frequency * 1.008
        }));
      });
      sources.push(addTone(output, time + 1.0, { type: "sine", duration: 1.1, gain: 0.052, frequency: 392, endFrequency: 523.25 }));
      sources.push(addNoise(output, time + 0.92, { duration: 0.45, gain: 0.045, frequency: 6200, filterType: "highpass", q: 0.55 }));
      return sources;
    }, details);
  }

  function playGameOver(details) {
    return playVoice(1.62, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.3, gain: 0.06, frequency: 1600, endFrequency: 280, filterType: "lowpass", q: 0.55 }),
        addTone(output, time, { type: "sawtooth", duration: 0.46, gain: 0.075, frequency: 293.66, endFrequency: 220 }),
        addTone(output, time + 0.34, { type: "triangle", duration: 0.5, gain: 0.085, frequency: 220, endFrequency: 164.81 }),
        addTone(output, time + 0.74, { type: "sine", duration: 0.78, gain: 0.105, frequency: 164.81, endFrequency: 55 }),
        addTone(output, time + 0.76, { type: "sine", duration: 0.72, gain: 0.045, frequency: 82.41, endFrequency: 41.2 })
      ];
    }, details);
  }

  function playButton(details) {
    return playVoice(0.1, function (output, time) {
      return [
        addTone(output, time, { type: "sine", duration: 0.065, gain: 0.042, frequency: 620, endFrequency: 920 }),
        addNoise(output, time, { duration: 0.025, gain: 0.026, frequency: 4500, filterType: "highpass", q: 1.2 })
      ];
    }, details);
  }

  function playAlert(details) {
    return playVoice(0.96, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.07, gain: 0.08, frequency: 4200, filterType: "highpass", q: 0.8 }),
        addTone(output, time, { type: "sawtooth", duration: 0.34, gain: 0.085, frequency: 155, endFrequency: 310 }),
        addTone(output, time + 0.08, { type: "square", duration: 0.2, gain: 0.048, frequency: 740, endFrequency: 980 }),
        addTone(output, time + 0.39, { type: "sawtooth", duration: 0.38, gain: 0.09, frequency: 174.61, endFrequency: 349.23 }),
        addTone(output, time + 0.48, { type: "square", duration: 0.23, gain: 0.05, frequency: 830, endFrequency: 1108 })
      ];
    }, details);
  }

  function playFootstep(details) {
    const weight = clamp(Number(details && details.weight) || 0.75, 0.35, 1.25);
    const pitch = randomBetween(0.88, 1.12);
    return playVoice(0.22, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.11, gain: 0.09 * weight, frequency: 720 * pitch, endFrequency: 210, filterType: "lowpass", q: 0.72 }),
        addTone(output, time, { type: "triangle", duration: 0.13, gain: 0.07 * weight, frequency: 92 * pitch, endFrequency: 42 }),
        addNoise(output, time + 0.045, { duration: 0.09, gain: 0.035 * weight, frequency: 2500, endFrequency: 900, q: 1.1 })
      ];
    }, details);
  }

  function playNearMiss(details) {
    let positionedDetails = details || {};
    if (typeof positionedDetails.pan !== "number") {
      positionedDetails = { pan: Math.random() < 0.5 ? -0.82 : 0.82 };
    }
    return playVoice(0.3, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.18, gain: 0.16, frequency: 1800, endFrequency: 7600, filterType: "highpass", q: 1.4 }),
        addTone(output, time + 0.018, { type: "sine", duration: 0.13, gain: 0.045, frequency: 1680, endFrequency: 490 }),
        addNoise(output, time + 0.13, { duration: 0.08, gain: 0.065, frequency: 5200, filterType: "highpass", q: 0.65 })
      ];
    }, positionedDetails);
  }

  function playObjectiveAmbience(details) {
    return playVoice(2.45, function (output, time) {
      return [
        addTone(output, time, { type: "sine", duration: 2.25, gain: 0.025, frequency: 55, endFrequency: 58 }),
        addTone(output, time + 0.18, { type: "sine", duration: 1.8, gain: 0.024, frequency: 220, endFrequency: 330 }),
        addTone(output, time + 0.62, { type: "triangle", duration: 0.48, gain: 0.027, frequency: 659.25, endFrequency: 987.77 }),
        addTone(output, time + 1.42, { type: "triangle", duration: 0.62, gain: 0.025, frequency: 523.25, endFrequency: 783.99 }),
        addNoise(output, time + 0.08, { duration: 2.1, gain: 0.016, frequency: 1900, endFrequency: 620, filterType: "lowpass", q: 0.5 })
      ];
    }, details);
  }

  const eventHandlers = {
    shot: playShot,
    enemyShot: playEnemyShot,
    melee: playMelee,
    empty: playEmpty,
    reload: playReload,
    hit: playHit,
    enemyDown: playEnemyDown,
    hurt: playHurt,
    pickup: playPickup,
    levelClear: playLevelClear,
    victory: playVictory,
    gameOver: playGameOver,
    button: playButton,
    alert: playAlert,
    footstep: playFootstep,
    nearMiss: playNearMiss,
    objectiveAmbience: playObjectiveAmbience
  };

  const eventCooldowns = {
    shot: 55,
    enemyShot: 75,
    melee: 140,
    empty: 90,
    reload: 180,
    hit: 25,
    enemyDown: 75,
    hurt: 80,
    pickup: 60,
    levelClear: 700,
    victory: 1200,
    gameOver: 900,
    button: 25,
    alert: 550,
    footstep: 85,
    nearMiss: 70,
    objectiveAmbience: 2100
  };

  function event(name, details) {
    return safely(function () {
      if (typeof name !== "string" || !eventHandlers[name]) return false;
      const time = Date.now();
      if (time - (lastEventAt[name] || 0) < eventCooldowns[name]) return false;
      const played = eventHandlers[name](details);
      if (played) lastEventAt[name] = time;
      return played;
    }, false);
  }

  function stopSpeech() {
    return safely(function () {
      speechUtterance = null;
      if (window.speechSynthesis) window.speechSynthesis.cancel();
      return true;
    }, false);
  }

  function findItalianVoice() {
    return safely(function () {
      const voicesList = window.speechSynthesis.getVoices();
      return voicesList.find(function (voice) {
        return String(voice.lang).toLowerCase() === "it-it";
      }) || voicesList.find(function (voice) {
        return String(voice.lang).toLowerCase().indexOf("it") === 0;
      }) || null;
    }, null);
  }

  function speakItalian(text) {
    return safely(function () {
      if (!initialized || muted || paused || typeof text !== "string" || !text.trim()) return false;
      if (!window.speechSynthesis || typeof window.SpeechSynthesisUtterance !== "function") return false;

      stopSpeech();
      const utterance = new window.SpeechSynthesisUtterance(text);
      const italianVoice = findItalianVoice();
      utterance.lang = "it-IT";
      utterance.rate = 0.88;
      utterance.pitch = 0.72;
      utterance.volume = 0.9;
      if (italianVoice) utterance.voice = italianVoice;
      utterance.addEventListener("end", function () {
        if (speechUtterance === utterance) speechUtterance = null;
      }, { once: true });
      utterance.addEventListener("error", function () {
        if (speechUtterance === utterance) speechUtterance = null;
      }, { once: true });
      speechUtterance = utterance;
      window.speechSynthesis.speak(utterance);
      return true;
    }, false);
  }

  function setMuted(value) {
    muted = Boolean(value);
    storeMute();
    if (muted) stopSpeech();
    setMasterLevel(0.025);
    return muted;
  }

  function setPaused(value) {
    paused = Boolean(value);
    if (paused) stopSpeech();
    setMasterLevel(paused ? 0.018 : 0.05);
    return paused;
  }

  function isMuted() {
    return muted;
  }

  window.GorillaAudio = Object.freeze({
    init: init,
    setMuted: setMuted,
    isMuted: isMuted,
    setPaused: setPaused,
    event: event,
    speakItalian: speakItalian,
    stopSpeech: stopSpeech
  });
}());
