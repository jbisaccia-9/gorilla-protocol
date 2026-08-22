(function () {
  "use strict";

  const STORAGE_KEY = "gorillaGoldenMute";
  const MAX_VOICES = 18;
  const SILENCE = 0.0001;
  const MASTER_LEVEL = 0.72;

  let context = null;
  let masterGain = null;
  let effectsGain = null;
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

  function buildAudioGraph() {
    const compressor = context.createDynamicsCompressor();
    masterGain = context.createGain();
    effectsGain = context.createGain();

    compressor.threshold.value = -10;
    compressor.knee.value = 12;
    compressor.ratio.value = 8;
    compressor.attack.value = 0.003;
    compressor.release.value = 0.18;
    masterGain.gain.value = muted || paused ? SILENCE : MASTER_LEVEL;
    effectsGain.gain.value = 0.78;

    effectsGain.connect(masterGain);
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

  function addTone(output, startTime, options) {
    const oscillator = context.createOscillator();
    const gain = context.createGain();
    const duration = Math.max(0.015, options.duration || 0.1);
    const startFrequency = Math.max(20, options.frequency || 220);
    const endFrequency = Math.max(20, options.endFrequency || startFrequency);

    oscillator.type = options.type || "triangle";
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
    filter.type = options.filterType || "bandpass";
    filter.frequency.value = options.frequency || 1200;
    filter.Q.value = options.q || 0.8;
    scheduleEnvelope(gain.gain, startTime, options.attack || 0.003, duration, options.gain || 0.1);
    source.connect(filter).connect(gain).connect(output);
    return source;
  }

  function playVoice(duration, build) {
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
      output.gain.value = 1;
      output.connect(effectsGain);
      const sources = build(output, startTime).filter(Boolean);
      return registerVoice(sources, output, startTime + duration + 0.035);
    }, false);
  }

  function playShot() {
    return playVoice(0.19, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.13, gain: 0.34, frequency: 1900, q: 0.58 }),
        addTone(output, time, { type: "sawtooth", duration: 0.18, gain: 0.18, frequency: 150, endFrequency: 42 })
      ];
    });
  }

  function playEnemyShot() {
    return playVoice(0.16, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.1, gain: 0.19, frequency: 1450, q: 0.8 }),
        addTone(output, time, { type: "square", duration: 0.14, gain: 0.1, frequency: 210, endFrequency: 72 })
      ];
    });
  }

  function playMelee() {
    return playVoice(0.24, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.18, gain: 0.2, frequency: 420, filterType: "lowpass", q: 0.7 }),
        addTone(output, time, { type: "triangle", duration: 0.2, gain: 0.13, frequency: 92, endFrequency: 45 })
      ];
    });
  }

  function playEmpty() {
    return playVoice(0.13, function (output, time) {
      return [
        addTone(output, time, { type: "square", duration: 0.045, gain: 0.07, frequency: 980, endFrequency: 620 }),
        addTone(output, time + 0.065, { type: "square", duration: 0.035, gain: 0.045, frequency: 720, endFrequency: 520 })
      ];
    });
  }

  function playReload() {
    return playVoice(0.72, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.08, gain: 0.12, frequency: 3500, q: 2.2 }),
        addTone(output, time + 0.2, { type: "square", duration: 0.07, gain: 0.07, frequency: 460, endFrequency: 740 }),
        addNoise(output, time + 0.43, { duration: 0.09, gain: 0.13, frequency: 2700, q: 2.8 }),
        addTone(output, time + 0.58, { type: "triangle", duration: 0.09, gain: 0.08, frequency: 620, endFrequency: 420 })
      ];
    });
  }

  function playHit() {
    return playVoice(0.14, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.11, gain: 0.15, frequency: 2300, q: 1.4 }),
        addTone(output, time, { duration: 0.12, gain: 0.07, frequency: 310, endFrequency: 110 })
      ];
    });
  }

  function playEnemyDown() {
    return playVoice(0.48, function (output, time) {
      return [
        addTone(output, time, { type: "sawtooth", duration: 0.42, gain: 0.09, frequency: 185, endFrequency: 48 }),
        addNoise(output, time + 0.04, { duration: 0.34, gain: 0.12, frequency: 620, filterType: "lowpass", q: 0.7 })
      ];
    });
  }

  function playHurt() {
    return playVoice(0.38, function (output, time) {
      return [
        addNoise(output, time, { duration: 0.32, gain: 0.22, frequency: 520, filterType: "lowpass", q: 0.65 }),
        addTone(output, time, { type: "sawtooth", duration: 0.3, gain: 0.1, frequency: 105, endFrequency: 58 })
      ];
    });
  }

  function playPickup() {
    return playVoice(0.42, function (output, time) {
      return [
        addTone(output, time, { duration: 0.14, gain: 0.08, frequency: 523, endFrequency: 660 }),
        addTone(output, time + 0.1, { duration: 0.16, gain: 0.08, frequency: 659, endFrequency: 784 }),
        addTone(output, time + 0.22, { duration: 0.17, gain: 0.09, frequency: 784, endFrequency: 1047 })
      ];
    });
  }

  function playLevelClear() {
    return playVoice(1.05, function (output, time) {
      return [
        addTone(output, time, { duration: 0.25, gain: 0.09, frequency: 392, endFrequency: 523 }),
        addTone(output, time + 0.22, { duration: 0.28, gain: 0.1, frequency: 523, endFrequency: 659 }),
        addTone(output, time + 0.48, { duration: 0.5, gain: 0.11, frequency: 659, endFrequency: 1047 })
      ];
    });
  }

  function playVictory() {
    return playVoice(1.8, function (output, time) {
      const notes = [262, 330, 392, 523, 659, 784];
      return notes.map(function (frequency, index) {
        return addTone(output, time + index * 0.18, {
          type: index < 3 ? "triangle" : "sine",
          duration: index === notes.length - 1 ? 0.78 : 0.25,
          gain: index === notes.length - 1 ? 0.13 : 0.085,
          frequency: frequency,
          endFrequency: index === notes.length - 1 ? 1047 : frequency * 1.01
        });
      });
    });
  }

  function playGameOver() {
    return playVoice(1.3, function (output, time) {
      return [
        addTone(output, time, { type: "sawtooth", duration: 0.38, gain: 0.09, frequency: 294, endFrequency: 220 }),
        addTone(output, time + 0.32, { type: "sawtooth", duration: 0.42, gain: 0.1, frequency: 220, endFrequency: 165 }),
        addTone(output, time + 0.68, { type: "triangle", duration: 0.55, gain: 0.12, frequency: 165, endFrequency: 73 })
      ];
    });
  }

  function playButton() {
    return playVoice(0.085, function (output, time) {
      return [addTone(output, time, {
        type: "square",
        duration: 0.06,
        gain: 0.045,
        frequency: 540,
        endFrequency: 720
      })];
    });
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
    button: playButton
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
    button: 25
  };

  function event(name) {
    return safely(function () {
      if (typeof name !== "string" || !eventHandlers[name]) return false;
      const time = Date.now();
      if (time - (lastEventAt[name] || 0) < eventCooldowns[name]) return false;
      const played = eventHandlers[name]();
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
