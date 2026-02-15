#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

function fail(message, extra) {
  const payload = { ok: false, error: message };
  if (extra) {
    payload.extra = extra;
  }
  process.stdout.write(JSON.stringify(payload));
  process.exit(0);
}

function ok(result) {
  process.stdout.write(JSON.stringify({ ok: true, result }));
  process.exit(0);
}

function parseInput() {
  const args = process.argv.slice(2);
  const base64FlagIndex = args.indexOf("--json-base64");
  if (base64FlagIndex !== -1 && base64FlagIndex + 1 < args.length) {
    const encoded = String(args[base64FlagIndex + 1] || "").trim();
    try {
      const decoded = Buffer.from(encoded, "base64").toString("utf8");
      return JSON.parse(decoded);
    } catch (error) {
      fail("Invalid base64 JSON payload.", String(error?.message || error));
    }
  }

  const jsonFlagIndex = args.indexOf("--json");
  if (jsonFlagIndex === -1 || jsonFlagIndex + 1 >= args.length) {
    fail("Missing --json payload.");
  }

  const rawPayload = String(args[jsonFlagIndex + 1] || "").trim();

  try {
    return JSON.parse(rawPayload);
  } catch (error) {
    try {
      const fallback = Function(`"use strict"; return (${rawPayload});`)();
      if (fallback && typeof fallback === "object") {
        return fallback;
      }
    } catch (_ignored) {
    }
    fail("Invalid JSON payload.", String(error?.message || error));
  }
}

function mulberry32(seed) {
  let t = seed >>> 0;
  return function random() {
    t += 0x6d2b79f5;
    let value = Math.imul(t ^ (t >>> 15), t | 1);
    value ^= value + Math.imul(value ^ (value >>> 7), value | 61);
    return ((value ^ (value >>> 14)) >>> 0) / 4294967296;
  };
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}

function _is_plain_object(value) {
  return value !== null && typeof value === "object" && !Array.isArray(value);
}

function _to_number(value, fallback) {
  if (typeof value === "number" && Number.isFinite(value)) {
    return value;
  }
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : fallback;
}

function _is_discrete_param(paramName) {
  return paramName === "waveType" || paramName === "terrain" || paramName === "shape";
}

function _sanitize_params_for_synth(synth, params, options) {
  const opts = _is_plain_object(options) ? options : {};
  const includeDefaults = !!opts.includeDefaults;
  const metadata = listParams(synth);
  const raw = _is_plain_object(params) ? params : {};
  const sanitized = {};

  for (const entry of metadata) {
    const hasValue = Object.prototype.hasOwnProperty.call(raw, entry.name);
    if (!hasValue && !includeDefaults) {
      continue;
    }

    const numeric = _to_number(raw[entry.name], entry.default);
    const clamped = clamp(numeric, entry.min, entry.max);
    sanitized[entry.name] = _is_discrete_param(entry.name) ? Math.floor(clamped) : clamped;
  }

  return sanitized;
}

function _mutate_params_for_synth(synth, baseParams, random) {
  const metadata = listParams(synth);
  const mutated = { ...baseParams };

  for (const entry of metadata) {
    const current = _to_number(mutated[entry.name], entry.default);
    const span = Math.max(0, entry.max - entry.min);
    if (_is_discrete_param(entry.name)) {
      if (random() < 0.45) {
        continue;
      }
      const direction = random() < 0.5 ? -1 : 1;
      mutated[entry.name] = Math.floor(clamp(Math.round(current) + direction, entry.min, entry.max));
      continue;
    }

    const delta = (random() * 2 - 1) * span * 0.15;
    mutated[entry.name] = clamp(current + delta, entry.min, entry.max);
  }

  return mutated;
}

function _resolve_seed(args) {
  if (Number.isInteger(args?.seed)) {
    return args.seed >>> 0;
  }
  const now = Date.now() >>> 0;
  const jitter = Math.floor(Math.random() * 0xffffffff) >>> 0;
  return (now ^ jitter ^ (process.pid >>> 0)) >>> 0;
}

const PRESETS = {
  bfxr: [
    ["pickup_coin", "Pickup/Coin"],
    ["laser_shoot", "Laser/Shoot"],
    ["explosion", "Explosion"],
    ["powerup", "Powerup"],
    ["hit_hurt", "Hit/Hurt"],
    ["jump", "Jump"],
    ["blip_select", "Blip/Select"],
    ["randomize", "Randomize"],
    ["mutate", "Mutate"],
  ],
  footsteppr: [
    ["randomize", "Randomize"],
    ["mutate", "Mutate"],
  ],
  transfxr: [
    ["randomize", "Randomize"],
    ["mutate", "Mutate"],
  ],
};

function listSynths() {
  return [
    { id: "bfxr", name: "Bfxr" },
    { id: "footsteppr", name: "Footsteppr" },
    { id: "transfxr", name: "Transfxr" },
  ];
}

function listPresets(synth) {
  const synthId = (synth || "bfxr").toLowerCase();
  const presets = PRESETS[synthId] || [];
  return presets.map(([id, name]) => ({ id, name, description: name, source: "phoenix_fallback" }));
}

function listParams(synth) {
  const synthId = (synth || "bfxr").toLowerCase();

  if (synthId === "footsteppr") {
    return [
      { name: "masterVolume", min: 0, max: 1, default: 0.5 },
      { name: "terrain", min: 0, max: 7, default: 0 },
      { name: "heel", min: 0, max: 1, default: 0.5 },
      { name: "ball", min: 0, max: 1, default: 0.5 },
      { name: "swiftness", min: 0, max: 1, default: 0.5 },
    ];
  }

  if (synthId === "transfxr") {
    return [
      { name: "masterVolume", min: 0, max: 1, default: 0.5 },
      { name: "frequency_start", min: 0, max: 1, default: 0.4 },
      { name: "frequency_end", min: 0, max: 1, default: 0.7 },
      { name: "swiftness", min: 0, max: 1, default: 0.5 },
      { name: "shape", min: 0, max: 3, default: 0 },
    ];
  }

  return [
    { name: "masterVolume", min: 0, max: 1, default: 0.5 },
    { name: "waveType", min: 0, max: 11, default: 0 },
    { name: "attackTime", min: 0, max: 1, default: 0 },
    { name: "sustainTime", min: 0, max: 1, default: 0.1 },
    { name: "decayTime", min: 0, max: 1, default: 0.2 },
    { name: "frequency_start", min: 0, max: 1, default: 0.4 },
    { name: "frequency_slide", min: -1, max: 1, default: 0 },
    { name: "vibratoDepth", min: 0, max: 1, default: 0 },
    { name: "vibratoSpeed", min: 0, max: 1, default: 0 },
    { name: "bitCrush", min: 0, max: 1, default: 0 },
  ];
}

function chooseDefaultsForPreset(synth, preset, random) {
  const defaults = {
    masterVolume: 0.6,
    attackTime: 0.0,
    sustainTime: 0.08,
    decayTime: 0.2,
    frequency_start: 0.4,
    frequency_end: 0.75,
    frequency_slide: 0.0,
    waveType: 0,
    swiftness: 0.5,
    terrain: 0,
  };

  const presetId = (preset || "").toLowerCase();
  const synthId = (synth || "bfxr").toLowerCase();

  if (synthId === "footsteppr") {
    defaults.attackTime = 0.002;
    defaults.sustainTime = 0.02;
    defaults.decayTime = 0.08;
    defaults.swiftness = 0.5;
    defaults.terrain = 0;
  }

  if (presetId === "pickup_coin") {
    defaults.waveType = 2;
    defaults.frequency_start = 0.75;
    defaults.frequency_slide = 0.2;
    defaults.sustainTime = 0.04;
    defaults.decayTime = 0.18;
  } else if (presetId === "laser_shoot") {
    defaults.waveType = 1;
    defaults.frequency_start = 0.72;
    defaults.frequency_slide = -0.45;
    defaults.sustainTime = 0.08;
    defaults.decayTime = 0.24;
  } else if (presetId === "explosion") {
    defaults.waveType = 3;
    defaults.frequency_start = 0.2;
    defaults.frequency_slide = -0.28;
    defaults.sustainTime = 0.14;
    defaults.decayTime = 0.36;
  } else if (presetId === "powerup") {
    defaults.waveType = 0;
    defaults.frequency_start = 0.3;
    defaults.frequency_slide = 0.35;
    defaults.sustainTime = 0.12;
    defaults.decayTime = 0.28;
  } else if (presetId === "hit_hurt") {
    defaults.waveType = 3;
    defaults.frequency_start = 0.5;
    defaults.frequency_slide = -0.35;
    defaults.sustainTime = 0.06;
    defaults.decayTime = 0.16;
  } else if (presetId === "jump") {
    defaults.waveType = 0;
    defaults.frequency_start = 0.45;
    defaults.frequency_slide = 0.22;
    defaults.sustainTime = 0.1;
    defaults.decayTime = 0.16;
  } else if (presetId === "blip_select") {
    defaults.waveType = 2;
    defaults.frequency_start = 0.6;
    defaults.frequency_slide = 0.0;
    defaults.sustainTime = 0.02;
    defaults.decayTime = 0.12;
  }

  if (presetId === "randomize" || presetId === "mutate") {
    defaults.waveType = Math.floor(random() * 4);
    defaults.frequency_start = random();
    defaults.frequency_slide = random() * 2 - 1;
    defaults.sustainTime = 0.02 + random() * 0.2;
    defaults.decayTime = 0.05 + random() * 0.35;
    defaults.swiftness = random();
    defaults.terrain = Math.floor(random() * 8);
  }

  return defaults;
}

function waveformValue(waveType, phase, noiseSample) {
  const wt = Math.floor(waveType);
  switch (wt) {
    case 0:
      return phase < 0.5 ? 1 : -1;
    case 1:
      return 1 - phase * 2;
    case 2:
      return Math.sin(phase * Math.PI * 2);
    case 4:
      return phase < 0.5 ? phase * 4 - 1 : 3 - phase * 4;
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    default:
      return noiseSample;
  }
}

function generateBfxrBuffer(params, random, sampleRate) {
  const attack = clamp(params.attackTime ?? 0.0, 0, 1) * 0.2;
  const sustain = 0.02 + clamp(params.sustainTime ?? 0.08, 0, 1) * 0.4;
  const decay = 0.04 + clamp(params.decayTime ?? 0.2, 0, 1) * 0.5;
  const duration = attack + sustain + decay;
  const totalSamples = Math.max(1, Math.floor(duration * sampleRate));
  const startFreq = 80 + clamp(params.frequency_start ?? 0.4, 0, 1) * 1800;
  const slide = clamp(params.frequency_slide ?? 0, -1, 1);
  const vibratoDepth = clamp(params.vibratoDepth ?? 0, 0, 1) * 0.025;
  const vibratoSpeed = 1 + clamp(params.vibratoSpeed ?? 0, 0, 1) * 20;
  const masterVolume = clamp(params.masterVolume ?? 0.6, 0, 1);

  let phase = 0;
  const buffer = new Float32Array(totalSamples);
  for (let i = 0; i < totalSamples; i++) {
    const t = i / sampleRate;
    let env;
    if (t < attack) {
      env = attack > 0 ? t / attack : 1;
    } else if (t < attack + sustain) {
      env = 1;
    } else {
      const dt = (t - attack - sustain) / Math.max(decay, 1e-6);
      env = 1 - clamp(dt, 0, 1);
    }

    const slideFactor = 1 + slide * (t / duration);
    const vib = 1 + Math.sin(t * Math.PI * 2 * vibratoSpeed) * vibratoDepth;
    const frequency = Math.max(20, startFreq * slideFactor * vib);
    phase = (phase + frequency / sampleRate) % 1;

    const noise = random() * 2 - 1;
    const waveform = waveformValue(params.waveType ?? 0, phase, noise);
    buffer[i] = clamp(waveform * env * masterVolume, -1, 1);
  }

  return buffer;
}

function generateFootstepBuffer(params, random, sampleRate) {
  const swiftness = clamp(params.swiftness ?? 0.5, 0, 1);
  const duration = 0.12 + (1 - swiftness) * 0.38;
  const totalSamples = Math.max(1, Math.floor(duration * sampleRate));
  const heel = clamp(params.heel ?? 0.5, 0, 1);
  const ball = clamp(params.ball ?? 0.5, 0, 1);
  const terrain = Math.floor(clamp(params.terrain ?? 0, 0, 8));
  const brightness = 0.25 + terrain * 0.08;
  const masterVolume = clamp(params.masterVolume ?? 0.6, 0, 1);

  const buffer = new Float32Array(totalSamples);
  for (let i = 0; i < totalSamples; i++) {
    const t = i / sampleRate;
    const normalized = t / duration;
    const heelEnv = Math.exp(-32 * Math.pow(normalized - 0.2, 2)) * heel;
    const ballEnv = Math.exp(-26 * Math.pow(normalized - 0.62, 2)) * ball;
    const env = clamp(heelEnv + ballEnv, 0, 1);
    const noise = random() * 2 - 1;
    const coarse = Math.sin(normalized * Math.PI * (8 + brightness * 10));
    buffer[i] = clamp((noise * 0.75 + coarse * 0.25) * env * masterVolume, -1, 1);
  }

  return buffer;
}

function generateTransfxrBuffer(params, random, sampleRate) {
  const swiftness = clamp(params.swiftness ?? 0.5, 0, 1);
  const duration = 0.18 + (1 - swiftness) * 0.5;
  const totalSamples = Math.max(1, Math.floor(duration * sampleRate));
  const startFreq = 60 + clamp(params.frequency_start ?? 0.35, 0, 1) * 1200;
  const endFreq = 60 + clamp(params.frequency_end ?? 0.7, 0, 1) * 2200;
  const masterVolume = clamp(params.masterVolume ?? 0.6, 0, 1);

  let phase = 0;
  const buffer = new Float32Array(totalSamples);
  for (let i = 0; i < totalSamples; i++) {
    const t = i / sampleRate;
    const normalized = t / duration;
    const freq = startFreq + (endFreq - startFreq) * normalized;
    phase = (phase + freq / sampleRate) % 1;

    const triangle = phase < 0.5 ? phase * 4 - 1 : 3 - phase * 4;
    const noise = (random() * 2 - 1) * 0.1;
    const env = 1 - normalized;
    buffer[i] = clamp((triangle + noise) * env * masterVolume, -1, 1);
  }

  return buffer;
}

function floatBufferToWavBytes(floatBuffer, sampleRate, bitDepth) {
  const numChannels = 1;
  const bytesPerSample = bitDepth / 8;
  const blockAlign = numChannels * bytesPerSample;
  const byteRate = sampleRate * blockAlign;
  const dataSize = floatBuffer.length * bytesPerSample;
  const buffer = Buffer.alloc(44 + dataSize);

  buffer.write("RIFF", 0);
  buffer.writeUInt32LE(36 + dataSize, 4);
  buffer.write("WAVE", 8);
  buffer.write("fmt ", 12);
  buffer.writeUInt32LE(16, 16);
  buffer.writeUInt16LE(1, 20);
  buffer.writeUInt16LE(numChannels, 22);
  buffer.writeUInt32LE(sampleRate, 24);
  buffer.writeUInt32LE(byteRate, 28);
  buffer.writeUInt16LE(blockAlign, 32);
  buffer.writeUInt16LE(bitDepth, 34);
  buffer.write("data", 36);
  buffer.writeUInt32LE(dataSize, 40);

  let offset = 44;
  for (let i = 0; i < floatBuffer.length; i++) {
    const sample = clamp(floatBuffer[i], -1, 1);
    const pcm = sample < 0 ? Math.floor(sample * 32768) : Math.floor(sample * 32767);
    buffer.writeInt16LE(pcm, offset);
    offset += 2;
  }

  return buffer;
}

function generateWav(args) {
  const synth = (args?.synth || "bfxr").toLowerCase();
  const preset = args?.preset || null;
  const presetId = typeof preset === "string" ? preset.toLowerCase() : "";
  const seed = _resolve_seed(args);
  const random = mulberry32(seed);

  const inputParams = _sanitize_params_for_synth(synth, args?.params);
  let mergedParams;
  if (presetId === "randomize") {
    mergedParams = _sanitize_params_for_synth(synth, chooseDefaultsForPreset(synth, "randomize", random), { includeDefaults: true });
  } else if (presetId === "mutate") {
    const hasInput = Object.keys(inputParams).length > 0;
    const baseParams = hasInput ? inputParams : _sanitize_params_for_synth(synth, chooseDefaultsForPreset(synth, null, random), { includeDefaults: true });
    mergedParams = _mutate_params_for_synth(synth, baseParams, random);
  } else {
    const defaults = _sanitize_params_for_synth(synth, chooseDefaultsForPreset(synth, preset, random), { includeDefaults: true });
    mergedParams = { ...defaults, ...inputParams };
  }

  const sampleRate = 44100;
  const bitDepth = 16;

  let floatBuffer;
  if (synth === "footsteppr") {
    floatBuffer = generateFootstepBuffer(mergedParams, random, sampleRate);
  } else if (synth === "transfxr") {
    floatBuffer = generateTransfxrBuffer(mergedParams, random, sampleRate);
  } else {
    floatBuffer = generateBfxrBuffer(mergedParams, random, sampleRate);
  }

  const wavBuffer = floatBufferToWavBytes(floatBuffer, sampleRate, bitDepth);
  const wavBase64 = wavBuffer.toString("base64");
  const includeDataUri = args?.returnDataUri !== false;
  const includeBase64 = args?.returnBase64 !== false;

  let savedTo = null;
  if (args?.outputPath) {
    const resolved = path.resolve(process.cwd(), String(args.outputPath));
    const root = path.resolve(process.cwd()) + path.sep;
    if (!resolved.startsWith(root)) {
      throw new Error("outputPath must be within the workspace directory.");
    }
    fs.mkdirSync(path.dirname(resolved), { recursive: true });
    fs.writeFileSync(resolved, wavBuffer);
    savedTo = resolved;
  }

  return {
    synth,
    preset,
    params: mergedParams,
    sampleRate,
    bitDepth,
    numSamples: floatBuffer.length,
    durationSeconds: floatBuffer.length / sampleRate,
    wavBase64: includeBase64 ? wavBase64 : null,
    dataUri: includeDataUri ? `data:audio/wav;base64,${wavBase64}` : null,
    outputPath: savedTo,
    engine: "phoenix_fallback_bridge",
    seed,
  };
}

function main() {
  const payload = parseInput();
  const command = String(payload?.command || "").trim().toLowerCase();
  const args = payload?.args || {};

  try {
    switch (command) {
      case "list_synths":
        ok(listSynths());
        return;
      case "list_presets":
        ok(listPresets(args?.synth));
        return;
      case "list_params":
        ok(listParams(args?.synth));
        return;
      case "generate_wav":
        ok(generateWav(args));
        return;
      default:
        fail(`Unknown command: ${command}`);
        return;
    }
  } catch (error) {
    fail(String(error?.message || error));
  }
}

main();
