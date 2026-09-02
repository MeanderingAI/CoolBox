/**
 * audio-synth.mjs
 * ───────────────
 * Browser-side JavaScript mirror of the CoolBox audio C++ libraries.
 *
 * Mirrors the C++ API surface from:
 *   _deliverables/libraries/groups/trekker/MISC/wave_generator/headers/wave_generator.hpp
 *     → namespace utils::wave_generator
 *   _deliverables/libraries/groups/audio_visual_group/VIDEO_ASSETS/audio_processing/headers/audio_processing.h
 *     → namespace trekker::audio
 *
 * Exports (mirroring C++ names exactly):
 *
 *   wave_generator:
 *     WavePattern, WaveConfig
 *     sample_at(time_seconds, pattern, config)
 *     generate_samples(sample_count, sample_rate_hz, pattern, config)
 *     generate_samples_for_duration(duration_seconds, sample_rate_hz, pattern, config)
 *
 *   audio_processing:
 *     SampleFormat, AudioBuffer
 *     mix(inputs, gains)
 *     peak_normalize(src, target_peak)
 *     rms_normalize(src, target_rms)
 *     apply_gain_envelope(src, gains)
 *     fade_in(src, fade_samples)
 *     fade_out(src, fade_samples)
 *     low_pass_filter(src, cutoff_hz)
 *     high_pass_filter(src, cutoff_hz)
 *     delay_effect(src, delay_samples, feedback)
 *     compress(src, threshold, ratio, attack_ms, release_ms, makeup_gain)
 *
 *   Web Audio bridge:
 *     AudioSynthEngine  — wraps the Web Audio API, routes all audio through
 *                         a master gain → analyser → destination chain.
 *                         Compatible with the AudioBuffer class above.
 */

'use strict';

// ─────────────────────────────────────────────────────────────────────────────
// utils::wave_generator
// ─────────────────────────────────────────────────────────────────────────────

/** Mirrors utils::wave_generator::WavePattern */
export const WavePattern = Object.freeze({
    Sine:             'Sine',
    Square:           'Square',
    Triangle:         'Triangle',
    Sawtooth:         'Sawtooth',
    ReverseSawtooth:  'ReverseSawtooth',
    Pulse:            'Pulse',
    WhiteNoise:       'WhiteNoise',
});

/** Mirrors utils::wave_generator::WaveConfig */
export class WaveConfig {
    constructor({
        amplitude       = 1.0,
        frequency_hz    = 1.0,
        phase_radians   = 0.0,
        offset          = 0.0,
        duty_cycle      = 0.5,
        noise_seed      = 0,
    } = {}) {
        this.amplitude      = amplitude;
        this.frequency_hz   = frequency_hz;
        this.phase_radians  = phase_radians;
        this.offset         = offset;
        this.duty_cycle     = duty_cycle;
        this.noise_seed     = noise_seed;
    }
}

/**
 * Mirrors utils::wave_generator::sample_at
 * Returns the signal value at the given time in seconds.
 */
export function sample_at(time_seconds, pattern, config = new WaveConfig()) {
    const { amplitude, frequency_hz, phase_radians, offset, duty_cycle } = config;
    const t = time_seconds * frequency_hz + phase_radians / (2 * Math.PI);
    const phase = t - Math.floor(t); // normalised [0, 1)
    let v;
    switch (pattern) {
        case WavePattern.Sine:
            v = Math.sin(2 * Math.PI * t);
            break;
        case WavePattern.Square:
            v = phase < 0.5 ? 1.0 : -1.0;
            break;
        case WavePattern.Triangle:
            v = phase < 0.5 ? 4 * phase - 1 : 3 - 4 * phase;
            break;
        case WavePattern.Sawtooth:
            v = 2 * phase - 1;
            break;
        case WavePattern.ReverseSawtooth:
            v = 1 - 2 * phase;
            break;
        case WavePattern.Pulse:
            v = phase < duty_cycle ? 1.0 : -1.0;
            break;
        case WavePattern.WhiteNoise:
            v = Math.random() * 2 - 1;
            break;
        default:
            v = 0;
    }
    return amplitude * v + offset;
}

/** Mirrors utils::wave_generator::generate_samples */
export function generate_samples(sample_count, sample_rate_hz, pattern, config = new WaveConfig()) {
    const out = new Array(sample_count);
    for (let i = 0; i < sample_count; i++) {
        out[i] = sample_at(i / sample_rate_hz, pattern, config);
    }
    return out;
}

/** Mirrors utils::wave_generator::generate_samples_for_duration */
export function generate_samples_for_duration(duration_seconds, sample_rate_hz, pattern, config = new WaveConfig()) {
    return generate_samples(Math.ceil(duration_seconds * sample_rate_hz), sample_rate_hz, pattern, config);
}

// ─────────────────────────────────────────────────────────────────────────────
// trekker::audio
// ─────────────────────────────────────────────────────────────────────────────

/** Mirrors trekker::audio::SampleFormat */
export const SampleFormat = Object.freeze({
    S16: 'S16',
    S32: 'S32',
    F32: 'F32',
    F64: 'F64',
});

/**
 * Mirrors trekker::audio::AudioBuffer
 * Planar (non-interleaved) multi-channel audio buffer.
 * planes[channel][sample_index] — all values normalised to [-1.0, 1.0].
 */
export class AudioBuffer {
    constructor({
        format       = SampleFormat.F32,
        sample_rate  = 48000,
        num_channels = 1,
        pts          = 0,
    } = {}) {
        this.format       = format;
        this.sample_rate  = sample_rate;
        this.num_channels = num_channels;
        this.pts          = pts;
        /** @type {number[][]} */
        this.planes = Array.from({ length: num_channels }, () => []);
    }

    num_samples() {
        return this.planes.length ? this.planes[0].length : 0;
    }

    duration_us() {
        if (!this.sample_rate) return 0;
        return (this.num_samples() * 1_000_000) / this.sample_rate;
    }

    /** Mirrors AudioBuffer::silent() */
    static silent(sample_rate, channels, num_samples, format = SampleFormat.F32) {
        const buf = new AudioBuffer({ format, sample_rate, num_channels: channels });
        buf.planes = Array.from({ length: channels }, () => new Array(num_samples).fill(0));
        return buf;
    }

    /** Mirrors AudioBuffer::clone() */
    clone() {
        const buf = new AudioBuffer({
            format:       this.format,
            sample_rate:  this.sample_rate,
            num_channels: this.num_channels,
            pts:          this.pts,
        });
        buf.planes = this.planes.map(ch => [...ch]);
        return buf;
    }

    /** Helper: wrap a flat samples array as a mono AudioBuffer. */
    static from_samples(samples, sample_rate = 48000) {
        const buf = new AudioBuffer({ sample_rate, num_channels: 1 });
        buf.planes[0] = Array.from(samples);
        return buf;
    }
}

/** Mirrors trekker::audio::mix */
export function mix(inputs, gains = []) {
    if (!inputs.length) return new AudioBuffer();
    const ref = inputs[0];
    const out = AudioBuffer.silent(ref.sample_rate, ref.num_channels, ref.num_samples());
    for (let i = 0; i < inputs.length; i++) {
        const g = gains[i] ?? 1.0;
        for (let ch = 0; ch < ref.num_channels; ch++) {
            const src = inputs[i].planes[ch];
            const dst = out.planes[ch];
            for (let s = 0; s < dst.length; s++) {
                dst[s] += (src?.[s] ?? 0) * g;
            }
        }
    }
    // clip to [-1, 1]
    for (const ch of out.planes) {
        for (let i = 0; i < ch.length; i++) ch[i] = Math.max(-1, Math.min(1, ch[i]));
    }
    return out;
}

/** Mirrors trekker::audio::peak_normalize */
export function peak_normalize(src, target_peak = 1.0) {
    const out = src.clone();
    let peak = 0;
    for (const ch of out.planes) for (const v of ch) if (Math.abs(v) > peak) peak = Math.abs(v);
    if (peak === 0) return out;
    const scale = target_peak / peak;
    for (const ch of out.planes) for (let i = 0; i < ch.length; i++) ch[i] *= scale;
    return out;
}

/** Mirrors trekker::audio::rms_normalize */
export function rms_normalize(src, target_rms = 0.1) {
    const out = src.clone();
    let sum = 0, count = 0;
    for (const ch of out.planes) { for (const v of ch) { sum += v * v; count++; } }
    const rms = count ? Math.sqrt(sum / count) : 0;
    if (rms === 0) return out;
    const scale = target_rms / rms;
    for (const ch of out.planes) for (let i = 0; i < ch.length; i++) ch[i] *= scale;
    return out;
}

/** Mirrors trekker::audio::apply_gain_envelope */
export function apply_gain_envelope(src, gains) {
    const out = src.clone();
    for (const ch of out.planes) {
        for (let i = 0; i < ch.length; i++) ch[i] *= (gains[i] ?? 1.0);
    }
    return out;
}

/** Mirrors trekker::audio::fade_in */
export function fade_in(src, fade_samples) {
    const out = src.clone();
    const n = Math.min(fade_samples, out.num_samples());
    for (const ch of out.planes) {
        for (let i = 0; i < n; i++) ch[i] *= i / n;
    }
    return out;
}

/** Mirrors trekker::audio::fade_out */
export function fade_out(src, fade_samples) {
    const out = src.clone();
    const total = out.num_samples();
    const n = Math.min(fade_samples, total);
    const start = total - n;
    for (const ch of out.planes) {
        for (let i = start; i < total; i++) ch[i] *= (total - 1 - i) / n;
    }
    return out;
}

/** Mirrors trekker::audio::low_pass_filter (first-order IIR) */
export function low_pass_filter(src, cutoff_hz) {
    const out = src.clone();
    const dt = 1.0 / src.sample_rate;
    const rc = 1.0 / (2 * Math.PI * cutoff_hz);
    const alpha = dt / (rc + dt);
    for (const ch of out.planes) {
        let prev = 0;
        for (let i = 0; i < ch.length; i++) {
            prev = prev + alpha * (ch[i] - prev);
            ch[i] = prev;
        }
    }
    return out;
}

/** Mirrors trekker::audio::high_pass_filter (first-order IIR) */
export function high_pass_filter(src, cutoff_hz) {
    const out = src.clone();
    const dt = 1.0 / src.sample_rate;
    const rc = 1.0 / (2 * Math.PI * cutoff_hz);
    const alpha = rc / (rc + dt);
    for (const ch of out.planes) {
        let prevIn = 0, prevOut = 0;
        for (let i = 0; i < ch.length; i++) {
            const y = alpha * (prevOut + ch[i] - prevIn);
            prevIn  = ch[i];
            prevOut = y;
            ch[i]   = y;
        }
    }
    return out;
}

/** Mirrors trekker::audio::delay_effect */
export function delay_effect(src, delay_samples, feedback) {
    const out = src.clone();
    for (const ch of out.planes) {
        const buf = new Array(delay_samples).fill(0);
        let pos = 0;
        for (let i = 0; i < ch.length; i++) {
            const delayed = buf[pos];
            const wet = ch[i] + delayed;
            buf[pos] = ch[i] + delayed * feedback;
            pos = (pos + 1) % delay_samples;
            ch[i] = Math.max(-1, Math.min(1, wet));
        }
    }
    return out;
}

/** Mirrors trekker::audio::compress (simple gain computer + smoother) */
export function compress(src, threshold, ratio, attack_ms, release_ms, makeup_gain = 1.0) {
    const out = src.clone();
    const attack_coef  = Math.exp(-1.0 / (src.sample_rate * attack_ms  / 1000));
    const release_coef = Math.exp(-1.0 / (src.sample_rate * release_ms / 1000));
    for (const ch of out.planes) {
        let env = 0;
        for (let i = 0; i < ch.length; i++) {
            const level = Math.abs(ch[i]);
            env = level > env
                ? attack_coef  * env + (1 - attack_coef)  * level
                : release_coef * env + (1 - release_coef) * level;
            const gain = env > threshold
                ? threshold + (env - threshold) / ratio - env
                : 0;
            ch[i] = Math.max(-1, Math.min(1, ch[i] * Math.pow(10, gain / 20) * makeup_gain));
        }
    }
    return out;
}

// ─────────────────────────────────────────────────────────────────────────────
// Web Audio bridge — AudioSynthEngine
// ─────────────────────────────────────────────────────────────────────────────

function _webaudio_type(pattern) {
    switch (pattern) {
        case WavePattern.Sine:            return 'sine';
        case WavePattern.Square:          return 'square';
        case WavePattern.Triangle:        return 'triangle';
        case WavePattern.Sawtooth:        return 'sawtooth';
        case WavePattern.ReverseSawtooth: return 'sawtooth';  // inverted via gain=-1
        case WavePattern.Pulse:           return 'square';    // duty_cycle approximated
        default:                          return 'sine';
    }
}

/**
 * AudioSynthEngine
 *
 * Wraps the Web Audio API. All oscillators route through:
 *   oscillator → oscillator_gain → filter → master_gain → analyser → destination
 *
 * This lets the analyser node capture all audio for oscilloscope / spectrum display.
 */
export class AudioSynthEngine {
    constructor() {
        this._ctx      = null;
        this._master   = null;   // GainNode
        this._analyser = null;   // AnalyserNode
        this._nodes    = new Map(); // id → { osc|noiseSource, gainNode, filter }
        this._nextId   = 1;
    }

    /** Lazily initialise the AudioContext on first access (requires user gesture). */
    get ctx() {
        if (!this._ctx) {
            this._ctx = new (window.AudioContext || window.webkitAudioContext)();

            this._master = this._ctx.createGain();
            this._master.gain.value = 0.8;

            this._analyser = this._ctx.createAnalyser();
            this._analyser.fftSize = 2048;
            this._analyser.smoothingTimeConstant = 0.75;

            this._master.connect(this._analyser);
            this._analyser.connect(this._ctx.destination);
        }
        return this._ctx;
    }

    /** The AnalyserNode — use for oscilloscope / spectrum visualisation. */
    get analyser() { void this.ctx; return this._analyser; }

    /** Resume a suspended AudioContext (call after a user gesture). */
    async resume() { await this.ctx.resume(); }

    /** Set the master output gain (0–1). */
    setMasterGain(v) {
        this.ctx;
        this._master.gain.setTargetAtTime(Math.max(0, Math.min(1, v)), this._ctx.currentTime, 0.01);
    }

    /**
     * Create a live oscillator.
     * Returns a handle: { id, setFrequency(hz), setGain(v), setType(WavePattern), setFilter(cutoff_hz), stop() }
     */
    createOscillator({ frequency_hz = 440, pattern = WavePattern.Sine, gain = 0.3 } = {}) {
        const ctx = this.ctx;

        const gainNode = ctx.createGain();
        gainNode.gain.value = gain;

        const filter = ctx.createBiquadFilter();
        filter.type = 'lowpass';
        filter.frequency.value = 20000; // fully open by default

        gainNode.connect(this._master);

        let osc = null;
        let noiseSource = null;

        if (pattern === WavePattern.WhiteNoise) {
            // Generate a 2-second looping noise buffer
            const bufLen = ctx.sampleRate * 2;
            const noiseBuf = ctx.createBuffer(1, bufLen, ctx.sampleRate);
            const ch = noiseBuf.getChannelData(0);
            for (let i = 0; i < bufLen; i++) ch[i] = Math.random() * 2 - 1;
            noiseSource = ctx.createBufferSource();
            noiseSource.buffer = noiseBuf;
            noiseSource.loop = true;
            noiseSource.connect(filter);
        } else if (pattern === WavePattern.ReverseSawtooth) {
            osc = ctx.createOscillator();
            osc.type = 'sawtooth';
            osc.frequency.value = frequency_hz;
            // Invert the polarity
            const invertGain = ctx.createGain();
            invertGain.gain.value = -1;
            osc.connect(invertGain);
            invertGain.connect(filter);
        } else {
            osc = ctx.createOscillator();
            osc.type = _webaudio_type(pattern);
            osc.frequency.value = frequency_hz;
            osc.connect(filter);
        }

        filter.connect(gainNode);

        if (osc)         osc.start();
        if (noiseSource) noiseSource.start();

        const id = this._nextId++;
        this._nodes.set(id, { osc, noiseSource, gainNode, filter });

        const self = this;
        return {
            id,
            setFrequency(hz) {
                if (osc) osc.frequency.setTargetAtTime(hz, ctx.currentTime, 0.005);
            },
            setGain(v) {
                gainNode.gain.setTargetAtTime(Math.max(0, v), ctx.currentTime, 0.005);
            },
            setType(p) {
                if (!osc || p === WavePattern.WhiteNoise || p === WavePattern.ReverseSawtooth) return;
                osc.type = _webaudio_type(p);
            },
            setFilter(cutoff) {
                filter.frequency.setTargetAtTime(
                    Math.max(20, Math.min(20000, cutoff)), ctx.currentTime, 0.01,
                );
            },
            stop() {
                gainNode.gain.setTargetAtTime(0, ctx.currentTime, 0.015);
                setTimeout(() => {
                    try { if (osc)         osc.stop();         } catch (_) {}
                    try { if (noiseSource) noiseSource.stop(); } catch (_) {}
                    self._nodes.delete(id);
                }, 120);
            },
        };
    }

    /**
     * Play a pre-rendered AudioBuffer (our custom class) once.
     * Returns a play ID that can be passed to stop().
     */
    play(buf, { gain = 1.0 } = {}) {
        const ctx = this.ctx;
        const webaudio = ctx.createBuffer(buf.num_channels, buf.num_samples(), buf.sample_rate);
        for (let ch = 0; ch < buf.num_channels; ch++) {
            webaudio.getChannelData(ch).set(buf.planes[ch]);
        }
        const src = ctx.createBufferSource();
        src.buffer = webaudio;
        const gainNode = ctx.createGain();
        gainNode.gain.value = gain;
        src.connect(gainNode);
        gainNode.connect(this._master);
        src.start();
        const id = this._nextId++;
        this._nodes.set(id, { src, gainNode });
        src.onended = () => this._nodes.delete(id);
        return id;
    }

    /** Stop a specific sound by its play/oscillator ID. */
    stop(id) {
        const n = this._nodes.get(id);
        if (!n) return;
        try { if (n.osc)         n.osc.stop();         } catch (_) {}
        try { if (n.noiseSource) n.noiseSource.stop(); } catch (_) {}
        try { if (n.src)         n.src.stop();         } catch (_) {}
        this._nodes.delete(id);
    }

    /** Stop all active sounds. */
    stopAll() {
        for (const id of [...this._nodes.keys()]) this.stop(id);
    }
}
