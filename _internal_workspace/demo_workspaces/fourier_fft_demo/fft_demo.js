const MODULE_SCRIPT_URL = '/demo-assets/fourier_fft_demo/module/fourier_tranforms.js';
const MODULE_WASM_BASE_URL = '/demo-assets/fourier_fft_demo/module/';
const MODULE_BUILD_URL = '/demo-assets/fourier_fft_demo/module/build';
const GIF_JS_URL = '/third_party/js_libs/gif/gif.js';
const GIF_WORKER_URL = '/third_party/js_libs/gif/gif.worker.js';
const TAU = Math.PI * 2;

const presetDefinitions = {
    warm: [
        { amplitude: 1.0, harmonic: 1, phase: 0.0 },
        { amplitude: 0.42, harmonic: 3, phase: 0.12 },
        { amplitude: 0.26, harmonic: 5, phase: -0.18 },
        { amplitude: 0.16, harmonic: 7, phase: 0.22 },
        { amplitude: 0.11, harmonic: 9, phase: -0.1 }
    ],
    bell: [
        { amplitude: 1.0, harmonic: 1, phase: 0.0 },
        { amplitude: 0.38, harmonic: 2, phase: 1.3 },
        { amplitude: 0.28, harmonic: 4, phase: 0.55 },
        { amplitude: 0.17, harmonic: 7, phase: 2.2 },
        { amplitude: 0.11, harmonic: 10, phase: 1.1 }
    ],
    pulse: [
        { amplitude: 1.0, harmonic: 1, phase: 0.0 },
        { amplitude: 0.78, harmonic: 2, phase: 0.0 },
        { amplitude: 0.52, harmonic: 4, phase: 0.0 },
        { amplitude: 0.34, harmonic: 8, phase: 0.0 },
        { amplitude: 0.2, harmonic: 12, phase: 0.0 }
    ],
    drift: [
        { amplitude: 1.0, harmonic: 1, phase: 0.0 },
        { amplitude: 0.31, harmonic: 2, phase: 0.45 },
        { amplitude: 0.24, harmonic: 3, phase: -0.72 },
        { amplitude: 0.15, harmonic: 5, phase: 1.6 },
        { amplitude: 0.09, harmonic: 11, phase: -2.2 }
    ]
};

const termPalette = ['#4ad7d1', '#ffd166', '#ff6b6b', '#8f7cff', '#7ae582', '#ff9f68', '#6ecbff', '#f472b6'];

const state = {
    module: null,
    moduleLoading: false,
    gifLibraryPromise: null,
    sampleCount: 256,
    visibleTerms: 5,
    speed: 1,
    preset: 'warm',
    time: 0,
    signal: [],
    referenceWave: [],
    reconstructedWave: [],
    fftBins: [],
    topTerms: [],
    fftMs: 0,
    dftMs: 0,
    lastTimestamp: 0,
    rebuildVersion: 0
};

const elements = {};
const contexts = {};

document.addEventListener('DOMContentLoaded', async () => {
    bindElements();
    bindControls();
    updateStatus('Checking Fourier Emscripten assets...', 'busy');
    drawLoadingFrame('Loading C++ Fourier module...');
    await initializeModule({ autoBuild: true });
    requestAnimationFrame(animate);
});

function bindElements() {
    elements.sampleCount = document.getElementById('sampleCount');
    elements.visibleTerms = document.getElementById('visibleTerms');
    elements.speed = document.getElementById('speed');
    elements.sampleCountValue = document.getElementById('sampleCountValue');
    elements.visibleTermsValue = document.getElementById('visibleTermsValue');
    elements.speedValue = document.getElementById('speedValue');
    elements.presetChips = Array.from(document.querySelectorAll('#presetChips .chip'));
    elements.dftOps = document.getElementById('dftOps');
    elements.fftOps = document.getElementById('fftOps');
    elements.speedup = document.getElementById('speedup');
    elements.fftTime = document.getElementById('fftTime');
    elements.dftTime = document.getElementById('dftTime');
    elements.peakFrequency = document.getElementById('peakFrequency');
    elements.phasorCaption = document.getElementById('phasorCaption');
    elements.waveCaption = document.getElementById('waveCaption');
    elements.engineStatus = document.getElementById('engineStatus');
    elements.engineLog = document.getElementById('engineLog');
    elements.btnBuildModule = document.getElementById('btnBuildModule');
    elements.btnExportGif = document.getElementById('btnExportGif');

    contexts.phasor = setupCanvas(document.getElementById('phasorCanvas'));
    contexts.wave = setupCanvas(document.getElementById('waveCanvas'));
    contexts.spectrum = setupCanvas(document.getElementById('spectrumCanvas'));

    window.addEventListener('resize', () => {
        contexts.phasor = setupCanvas(document.getElementById('phasorCanvas'));
        contexts.wave = setupCanvas(document.getElementById('waveCanvas'));
        contexts.spectrum = setupCanvas(document.getElementById('spectrumCanvas'));
        drawFrame();
    });
}

function bindControls() {
    elements.sampleCount.addEventListener('input', async () => {
        state.sampleCount = Number(elements.sampleCount.value);
        await rebuildScene();
    });

    elements.visibleTerms.addEventListener('input', () => {
        state.visibleTerms = Number(elements.visibleTerms.value);
        refreshDerivedSeries();
        updateReadout();
        drawFrame();
    });

    elements.speed.addEventListener('input', () => {
        state.speed = Number(elements.speed.value) / 100;
        updateReadout();
    });

    elements.presetChips.forEach((chip) => {
        chip.addEventListener('click', async () => {
            state.preset = chip.dataset.preset;
            elements.presetChips.forEach((entry) => entry.classList.toggle('active', entry === chip));
            await rebuildScene();
        });
    });

    elements.btnBuildModule.addEventListener('click', async () => {
        await initializeModule({ forceBuild: true });
    });

    elements.btnExportGif.addEventListener('click', async () => {
        await exportGif();
    });
}

function setupCanvas(canvas) {
    const parent = canvas.parentElement;
    const dpr = window.devicePixelRatio || 1;
    const width = Math.max(300, Math.floor(parent.clientWidth));
    const height = Math.max(220, Math.floor(parent.clientHeight));
    canvas.width = Math.floor(width * dpr);
    canvas.height = Math.floor(height * dpr);
    const ctx = canvas.getContext('2d');
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    return { canvas, ctx, width, height };
}

async function initializeModule({ autoBuild = false, forceBuild = false } = {}) {
    if (state.moduleLoading) {
        return;
    }

    state.moduleLoading = true;
    elements.btnBuildModule.disabled = true;
    elements.btnExportGif.disabled = true;

    try {
        let assetsReady = await checkModuleAssets();
        if (forceBuild || (autoBuild && !assetsReady)) {
            await buildModuleAssets();
            assetsReady = await checkModuleAssets();
        }
        if (!assetsReady) {
            throw new Error('fourier_tranforms.js/.wasm not available');
        }

        await ensureModuleScript();
        if (typeof window.createFourierTransformsModule !== 'function') {
            throw new Error('createFourierTransformsModule was not exported');
        }

        state.module = await window.createFourierTransformsModule({
            locateFile: (path) => MODULE_WASM_BASE_URL + path
        });

        updateStatus('C++ Fourier module loaded', 'ready');
        appendLog('Loaded Fourier Emscripten module from fourier_tranforms.js/.wasm.');
        elements.btnExportGif.disabled = false;
        await rebuildScene();
    } catch (error) {
        state.module = null;
        updateStatus('C++ Fourier module unavailable', 'error');
        appendLog(`[ERR] ${error.message}`);
        drawLoadingFrame('Build the Fourier WASM module to start the demo.');
    } finally {
        state.moduleLoading = false;
        elements.btnBuildModule.disabled = false;
        elements.btnExportGif.disabled = !state.module;
    }
}

async function checkModuleAssets() {
    const [hasJs, hasWasm] = await Promise.all([
        assetExists(MODULE_SCRIPT_URL),
        assetExists(MODULE_WASM_BASE_URL + 'fourier_tranforms.wasm')
    ]);
    return hasJs && hasWasm;
}

async function assetExists(url) {
    try {
        const response = await fetch(url, { method: 'GET', cache: 'no-store' });
        return response.ok;
    } catch (_) {
        return false;
    }
}

async function buildModuleAssets() {
    updateStatus('Building Fourier Emscripten assets...', 'busy');
    appendLog('POST ' + MODULE_BUILD_URL);
    const response = await fetch(MODULE_BUILD_URL, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' }
    });
    const data = await response.json();
    const tail = String(data.output || '').split('\n').slice(-20).join('\n') || '(no build output)';
    elements.engineLog.textContent = tail;

    if (!response.ok || !data.success) {
        throw new Error('Fourier asset build failed');
    }
}

async function ensureModuleScript() {
    const oldScript = document.getElementById('fourier-module-script');
    if (oldScript) {
        oldScript.remove();
    }
    await new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.id = 'fourier-module-script';
        script.src = MODULE_SCRIPT_URL;
        script.async = true;
        script.onload = resolve;
        script.onerror = () => reject(new Error('Failed to load fourier_tranforms.js'));
        document.head.appendChild(script);
    });
}

async function rebuildScene() {
    state.time = 0;
    state.lastTimestamp = 0;
    state.signal = buildSignal(state.sampleCount, presetDefinitions[state.preset]);
    state.referenceWave = state.signal.slice();
    state.rebuildVersion += 1;
    const currentVersion = state.rebuildVersion;

    if (!state.module) {
        updateReadout();
        drawLoadingFrame('Fourier module not loaded.');
        return;
    }

    updateStatus('Computing DFT and FFT in C++...', 'busy');

    const fftInput = toCppDoubleVector(state.signal);
    const fftStart = performance.now();
    const fftVector = state.module.real_fft_spectrum(fftInput);
    const fftMs = performance.now() - fftStart;
    disposeVector(fftInput);

    const dftInput = toCppDoubleVector(state.signal);
    const dftStart = performance.now();
    const dftVector = state.module.dft_spectrum(dftInput);
    const dftMs = performance.now() - dftStart;
    disposeVector(dftInput);

    if (currentVersion !== state.rebuildVersion) {
        disposeVector(fftVector);
        disposeVector(dftVector);
        return;
    }

    state.fftBins = vectorToArray(fftVector).map((point, index) => ({
        index,
        real: point.real,
        imag: point.imag,
        magnitude: point.magnitude,
        phase: point.phase
    }));
    state.fftMs = fftMs;
    state.dftMs = dftMs;

    disposeVector(fftVector);
    disposeVector(dftVector);

    refreshDerivedSeries();
    updateStatus('C++ Fourier module loaded', 'ready');
    updateReadout();
    drawFrame();
}

function refreshDerivedSeries() {
    state.topTerms = selectTopTerms(state.fftBins, state.sampleCount, state.visibleTerms);
    state.reconstructedWave = new Array(state.sampleCount);
    for (let i = 0; i < state.sampleCount; i += 1) {
        state.reconstructedWave[i] = evaluatePoint(i / state.sampleCount, state.topTerms);
    }
}

function buildSignal(sampleCount, terms) {
    const samples = new Array(sampleCount);
    for (let i = 0; i < sampleCount; i += 1) {
        const t = i / sampleCount;
        let value = 0;
        for (const term of terms) {
            value += term.amplitude * Math.sin(TAU * term.harmonic * t + term.phase);
        }
        samples[i] = value;
    }
    return normalizeSignal(samples);
}

function normalizeSignal(samples) {
    let peak = 0;
    for (const sample of samples) {
        peak = Math.max(peak, Math.abs(sample));
    }
    const scale = peak > 0 ? 0.92 / peak : 1;
    return samples.map((value) => value * scale);
}

function selectTopTerms(bins, sampleCount, count) {
    const usable = bins
        .slice(1, Math.floor(sampleCount / 2))
        .map((bin) => ({
            bin: bin.index,
            amplitude: (2 * bin.magnitude) / sampleCount,
            phase: bin.phase,
            color: termPalette[(bin.index - 1) % termPalette.length]
        }))
        .filter((term) => term.amplitude > 0.001)
        .sort((left, right) => right.amplitude - left.amplitude)
        .slice(0, count);

    return usable.sort((left, right) => left.bin - right.bin);
}

function updateReadout() {
    elements.sampleCountValue.textContent = String(state.sampleCount);
    elements.visibleTermsValue.textContent = String(state.visibleTerms);
    elements.speedValue.textContent = `${state.speed.toFixed(2)}x`;

    const dftOps = state.sampleCount * state.sampleCount;
    const fftOps = Math.max(1, Math.round(state.sampleCount * Math.log2(state.sampleCount)));
    const peakIndex = findPeakIndex(state.fftBins);
    const measuredSpeedup = state.fftMs > 0 ? state.dftMs / state.fftMs : 0;

    elements.dftOps.textContent = formatNumber(dftOps);
    elements.fftOps.textContent = formatNumber(fftOps);
    elements.speedup.textContent = `${measuredSpeedup.toFixed(1)}x`;
    elements.fftTime.textContent = `${state.fftMs.toFixed(2)} ms`;
    elements.dftTime.textContent = `${state.dftMs.toFixed(2)} ms`;
    elements.peakFrequency.textContent = `${peakIndex.toFixed(1)} bins`;
    elements.waveCaption.textContent = `${state.visibleTerms} terms · ${state.sampleCount} samples`;
}

function animate(timestamp) {
    if (!state.lastTimestamp) {
        state.lastTimestamp = timestamp;
    }
    const dt = (timestamp - state.lastTimestamp) / 1000;
    state.lastTimestamp = timestamp;
    state.time = (state.time + dt * state.speed * 0.18) % 1;
    drawFrame();
    requestAnimationFrame(animate);
}

function drawFrame() {
    if (!contexts.phasor || !contexts.wave || !contexts.spectrum) {
        return;
    }
    drawPhasor(contexts.phasor);
    drawSpectrum(contexts.spectrum);
    drawWave(contexts.wave);
}

function drawLoadingFrame(message) {
    [contexts.phasor, contexts.spectrum, contexts.wave].forEach((context) => {
        if (!context) {
            return;
        }
        const { ctx, width, height } = context;
        ctx.clearRect(0, 0, width, height);
        drawGrid(ctx, width, height, 30);
        ctx.fillStyle = 'rgba(232, 241, 255, 0.86)';
        ctx.font = '15px Segoe UI, sans-serif';
        ctx.fillText(message, 22, 34);
    });
}

function drawPhasor({ ctx, width, height }) {
    ctx.clearRect(0, 0, width, height);
    drawGrid(ctx, width, height, 28);

    const cx = width * 0.34;
    const cy = height * 0.5;
    const totalAmplitude = state.topTerms.reduce((sum, term) => sum + term.amplitude, 0) || 1;
    const scale = Math.min(width, height) * 0.18 / totalAmplitude;
    let x = cx;
    let y = cy;
    const point = evaluatePoint(state.time, state.topTerms);

    ctx.strokeStyle = 'rgba(232, 241, 255, 0.12)';
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, height);
    ctx.moveTo(0, y);
    ctx.lineTo(width, y);
    ctx.stroke();

    state.topTerms.forEach((term) => {
        const radius = term.amplitude * scale;
        const angle = TAU * term.bin * state.time + term.phase - Math.PI / 2;
        const nextX = x + Math.cos(angle) * radius;
        const nextY = y + Math.sin(angle) * radius;

        ctx.strokeStyle = withAlpha(term.color, 0.22);
        ctx.lineWidth = 1.1;
        ctx.beginPath();
        ctx.arc(x, y, radius, 0, TAU);
        ctx.stroke();

        ctx.strokeStyle = term.color;
        ctx.lineWidth = 2.2;
        ctx.beginPath();
        ctx.moveTo(x, y);
        ctx.lineTo(nextX, nextY);
        ctx.stroke();

        ctx.fillStyle = term.color;
        ctx.beginPath();
        ctx.arc(nextX, nextY, 4, 0, TAU);
        ctx.fill();

        x = nextX;
        y = nextY;
    });

    ctx.strokeStyle = 'rgba(74, 215, 209, 0.6)';
    ctx.setLineDash([6, 6]);
    ctx.beginPath();
    ctx.moveTo(x, y);
    ctx.lineTo(width * 0.92, y);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.fillStyle = '#ffffff';
    ctx.beginPath();
    ctx.arc(x, y, 5.2, 0, TAU);
    ctx.fill();

    ctx.fillStyle = 'rgba(232, 241, 255, 0.84)';
    ctx.font = '12px Cascadia Code, Consolas, monospace';
    ctx.fillText(`y(t) = ${point.toFixed(3)}`, 14, 22);
    elements.phasorCaption.textContent = `t = ${state.time.toFixed(3)}`;
}

function drawWave({ ctx, width, height }) {
    ctx.clearRect(0, 0, width, height);
    drawGrid(ctx, width, height, 32);

    const left = 24;
    const right = width - 18;
    const top = 18;
    const bottom = height - 24;
    const midY = (top + bottom) / 2;
    const waveWidth = right - left;

    ctx.strokeStyle = 'rgba(232, 241, 255, 0.18)';
    ctx.beginPath();
    ctx.moveTo(left, midY);
    ctx.lineTo(right, midY);
    ctx.stroke();

    ctx.strokeStyle = 'rgba(255, 209, 102, 0.24)';
    ctx.lineWidth = 1.6;
    ctx.beginPath();
    state.referenceWave.forEach((sample, index) => {
        const x = left + (index / Math.max(1, state.referenceWave.length - 1)) * waveWidth;
        const y = midY - sample * (bottom - top) * 0.42;
        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });
    ctx.stroke();

    ctx.strokeStyle = '#4ad7d1';
    ctx.lineWidth = 2.6;
    ctx.beginPath();
    state.reconstructedWave.forEach((sample, index) => {
        const x = left + (index / Math.max(1, state.reconstructedWave.length - 1)) * waveWidth;
        const y = midY - sample * (bottom - top) * 0.42;
        if (index === 0) {
            ctx.moveTo(x, y);
        } else {
            ctx.lineTo(x, y);
        }
    });
    ctx.stroke();

    const markerX = left + state.time * waveWidth;
    const markerY = midY - evaluatePoint(state.time, state.topTerms) * (bottom - top) * 0.42;
    ctx.fillStyle = '#ffd166';
    ctx.beginPath();
    ctx.arc(markerX, markerY, 5, 0, TAU);
    ctx.fill();
}

function drawSpectrum({ ctx, width, height }) {
    ctx.clearRect(0, 0, width, height);
    drawGrid(ctx, width, height, 26);

    const bins = state.fftBins.slice(1, Math.min(25, Math.floor(state.fftBins.length / 2)));
    let peak = 0;
    bins.forEach((bin) => {
        peak = Math.max(peak, bin.magnitude);
    });

    const visibleBins = new Set(state.topTerms.map((term) => term.bin));
    const chartBottom = height - 26;
    const chartTop = 20;
    const barAreaHeight = chartBottom - chartTop;
    const barWidth = Math.max(10, (width - 32) / Math.max(1, bins.length) - 6);

    bins.forEach((bin, index) => {
        const normalized = peak > 0 ? bin.magnitude / peak : 0;
        const x = 16 + index * (barWidth + 6);
        const barHeight = normalized * barAreaHeight;
        const y = chartBottom - barHeight;
        const color = visibleBins.has(bin.index) ? termPalette[(bin.index - 1) % termPalette.length] : '#8ea6c5';

        ctx.fillStyle = withAlpha(color, 0.18);
        ctx.fillRect(x, chartTop, barWidth, barAreaHeight);

        const gradient = ctx.createLinearGradient(0, y, 0, chartBottom);
        gradient.addColorStop(0, color);
        gradient.addColorStop(1, withAlpha(color, 0.3));
        ctx.fillStyle = gradient;
        ctx.fillRect(x, y, barWidth, barHeight);

        ctx.fillStyle = 'rgba(232, 241, 255, 0.72)';
        ctx.font = '11px Cascadia Code, Consolas, monospace';
        ctx.fillText(String(bin.index), x, chartBottom + 14);
    });
}

function evaluatePoint(t, terms) {
    let value = 0;
    for (const term of terms) {
        value += term.amplitude * Math.cos(TAU * term.bin * t + term.phase);
    }
    return value;
}

function findPeakIndex(bins) {
    let bestIndex = 0;
    let bestValue = -Infinity;
    bins.slice(1, Math.floor(bins.length / 2)).forEach((bin) => {
        if (bin.magnitude > bestValue) {
            bestValue = bin.magnitude;
            bestIndex = bin.index;
        }
    });
    return bestIndex;
}

function toCppDoubleVector(values) {
    const vector = new state.module.VectorDouble_Fourier();
    values.forEach((value) => vector.push_back(value));
    return vector;
}

function vectorToArray(vector) {
    const values = [];
    for (let index = 0; index < vector.size(); index += 1) {
        values.push(vector.get(index));
    }
    return values;
}

function disposeVector(vector) {
    if (vector && typeof vector.delete === 'function') {
        vector.delete();
    }
}

function updateStatus(text, mode) {
    elements.engineStatus.textContent = text;
    elements.engineStatus.classList.remove('ready', 'busy', 'error');
    if (mode) {
        elements.engineStatus.classList.add(mode);
    }
}

function appendLog(message) {
    elements.engineLog.textContent = message;
}

function formatNumber(value) {
    return new Intl.NumberFormat('en-US').format(value);
}

function drawGrid(ctx, width, height, step) {
    ctx.strokeStyle = 'rgba(172, 203, 255, 0.08)';
    ctx.lineWidth = 1;
    ctx.beginPath();
    for (let x = 0; x <= width; x += step) {
        ctx.moveTo(x, 0);
        ctx.lineTo(x, height);
    }
    for (let y = 0; y <= height; y += step) {
        ctx.moveTo(0, y);
        ctx.lineTo(width, y);
    }
    ctx.stroke();
}

function withAlpha(hex, alpha) {
    const clean = hex.replace('#', '');
    const value = clean.length === 3 ? clean.split('').map((char) => char + char).join('') : clean;
    const red = parseInt(value.slice(0, 2), 16);
    const green = parseInt(value.slice(2, 4), 16);
    const blue = parseInt(value.slice(4, 6), 16);
    return `rgba(${red}, ${green}, ${blue}, ${alpha})`;
}

function loadGifLibrary() {
    if (window.GIF) {
        return Promise.resolve(window.GIF);
    }
    if (state.gifLibraryPromise) {
        return state.gifLibraryPromise;
    }
    state.gifLibraryPromise = new Promise((resolve, reject) => {
        const existingScript = document.querySelector('script[data-gif-js]');
        if (existingScript) {
            existingScript.addEventListener('load', () => resolve(window.GIF));
            existingScript.addEventListener('error', () => reject(new Error('Failed to load GIF encoder.')));
            return;
        }
        const script = document.createElement('script');
        script.src = GIF_JS_URL;
        script.async = true;
        script.dataset.gifJs = 'true';
        script.onload = () => resolve(window.GIF);
        script.onerror = () => reject(new Error('Failed to load GIF encoder.'));
        document.head.appendChild(script);
    });
    return state.gifLibraryPromise;
}

async function exportGif() {
    if (!state.module) {
        return;
    }

    const previousTime = state.time;
    elements.btnExportGif.disabled = true;
    updateStatus('Rendering GIF export...', 'busy');

    try {
        const GIF = await loadGifLibrary();
        const exportWidth = 1280;
        const exportHeight = 760;
        const gif = new GIF({
            workers: 2,
            quality: 10,
            workerScript: GIF_WORKER_URL,
            width: exportWidth,
            height: exportHeight,
            repeat: 0,
            background: '#08111b'
        });

        const frameCount = 48;
        const gifBlob = await new Promise((resolve, reject) => {
            gif.on('finished', resolve);
            gif.on('abort', () => reject(new Error('GIF render was aborted.')));
            gif.on('error', reject);

            for (let frame = 0; frame < frameCount; frame += 1) {
                state.time = frame / frameCount;
                drawFrame();
                gif.addFrame(createExportFrame(exportWidth, exportHeight), { copy: true, delay: 70 });
            }

            gif.render();
        });

        const objectUrl = URL.createObjectURL(gifBlob);
        const link = document.createElement('a');
        link.style.display = 'none';
        link.download = `fourier_fft_${state.preset}_${state.sampleCount}.gif`;
        link.href = objectUrl;
        document.body.appendChild(link);
        link.click();
        setTimeout(() => {
            URL.revokeObjectURL(objectUrl);
            link.remove();
        }, 1500);
        appendLog('GIF export complete.');
    } catch (error) {
        appendLog(`[ERR] GIF export failed: ${error.message}`);
    } finally {
        state.time = previousTime;
        drawFrame();
        updateStatus('C++ Fourier module loaded', 'ready');
        elements.btnExportGif.disabled = !state.module;
    }
}

function createExportFrame(width, height) {
    const canvas = document.createElement('canvas');
    canvas.width = width;
    canvas.height = height;
    const ctx = canvas.getContext('2d');

    const gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, '#08111b');
    gradient.addColorStop(1, '#0a1422');
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, width, height);

    ctx.fillStyle = 'rgba(232, 241, 255, 0.96)';
    ctx.font = '700 28px Segoe UI';
    ctx.fillText('Fourier Circuit and FFT Demo', 36, 44);
    ctx.font = '15px Segoe UI';
    ctx.fillStyle = 'rgba(232, 241, 255, 0.68)';
    ctx.fillText(`Preset: ${state.preset}  |  Samples: ${state.sampleCount}  |  Terms: ${state.visibleTerms}`, 36, 70);

    drawCard(ctx, 28, 94, 540, 300, contexts.phasor.canvas, 'Phasor Circuit');
    drawCard(ctx, 590, 94, 662, 300, contexts.spectrum.canvas, 'FFT Spectrum');
    drawCard(ctx, 28, 416, 1224, 300, contexts.wave.canvas, 'Waveform Reconstruction');

    return canvas;
}

function drawCard(ctx, x, y, width, height, sourceCanvas, title) {
    ctx.fillStyle = 'rgba(11, 23, 37, 0.92)';
    ctx.strokeStyle = 'rgba(172, 203, 255, 0.14)';
    ctx.lineWidth = 1;
    roundRect(ctx, x, y, width, height, 18);
    ctx.fill();
    ctx.stroke();

    ctx.fillStyle = 'rgba(232, 241, 255, 0.9)';
    ctx.font = '700 18px Segoe UI';
    ctx.fillText(title, x + 18, y + 28);
    ctx.drawImage(sourceCanvas, x + 14, y + 44, width - 28, height - 58);
}

function roundRect(ctx, x, y, width, height, radius) {
    ctx.beginPath();
    ctx.moveTo(x + radius, y);
    ctx.lineTo(x + width - radius, y);
    ctx.quadraticCurveTo(x + width, y, x + width, y + radius);
    ctx.lineTo(x + width, y + height - radius);
    ctx.quadraticCurveTo(x + width, y + height, x + width - radius, y + height);
    ctx.lineTo(x + radius, y + height);
    ctx.quadraticCurveTo(x, y + height, x, y + height - radius);
    ctx.lineTo(x, y + radius);
    ctx.quadraticCurveTo(x, y, x + radius, y);
    ctx.closePath();
}
