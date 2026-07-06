/**
 * graphics-chart.mjs
 * ──────────────────
 * Browser-side JavaScript mirror of the CoolBox GRAPHICS/charts library.
 *
 * Mirrors the C++ API surface from:
 *   _deliverables/libraries/groups/app_builder/GRAPHICS/charts/headers/graphics.h
 *
 * Classes exposed:
 *   GraphType  (enum-like object)    Line | Bar | Scatter
 *   Colors     (named colour palette matching Colors:: namespace)
 *   Canvas     (pixel canvas with drawing primitives)
 *   Graph      (chart renderer — matches graphics::Graph)
 *   Table      (table renderer  — matches graphics::Table)
 *   FunctionPlot  (y = f(x))
 *   ParametricPlot
 *   PolarPlot
 *   HistogramPlot
 *
 * Custom element:
 *   <graphics-chart> — declarative wrapper around Graph/Table/FunctionPlot etc.
 *
 * Usage (imperative):
 *   import { Graph, GraphType, Colors } from '/extensions/graphics-chart.mjs';
 *   const g = new Graph(800, 400, GraphType.Line);
 *   g.set_title('Training Loss');
 *   g.add_series({ label: 'train', x_values: [1,2,3], y_values: [0.9,0.4,0.1], color: Colors.Blue });
 *   g.render_to_canvas(document.querySelector('canvas'));
 *
 * Usage (declarative):
 *   <graphics-chart type="line" title="Loss" width="600" height="350"
 *       series='[{"label":"train","x":[1,2,3],"y":[0.9,0.4,0.1]}]'></graphics-chart>
 */

'use strict';

// ── Color helpers ─────────────────────────────────────────────────────────────
export function rgba(r, g, b, a = 255) { return { r, g, b, a }; }
export function rgb(r, g, b)           { return rgba(r, g, b, 255); }
function cssColor(c) {
    return `rgba(${c.r},${c.g},${c.b},${(c.a / 255).toFixed(3)})`;
}

export const Colors = Object.freeze({
    Black:     rgb(  0,   0,   0),
    White:     rgb(255, 255, 255),
    Red:       rgb(220,  50,  50),
    Green:     rgb( 50, 180,  50),
    Blue:      rgb( 50,  90, 220),
    Orange:    rgb(230, 150,  30),
    Purple:    rgb(150,  50, 200),
    Cyan:      rgb( 50, 200, 200),
    Gray:      rgb(180, 180, 180),
    DarkGray:  rgb(100, 100, 100),
    LightGray: rgb(230, 230, 230),
});

export const GraphType = Object.freeze({ Line: 'Line', Bar: 'Bar', Scatter: 'Scatter' });

// ── DataSeries ────────────────────────────────────────────────────────────────
/** Mirrors struct DataSeries in graphics.h */
export function DataSeries(label, x_values, y_values, color = Colors.Blue) {
    return { label, x_values, y_values, color };
}

// ── Internal drawing helpers ──────────────────────────────────────────────────
const MARGIN = { top: 48, right: 24, bottom: 52, left: 64 };

function plotArea(W, H, m = MARGIN) {
    return { x: m.left, y: m.top, w: W - m.left - m.right, h: H - m.top - m.bottom };
}

function niceRange(values) {
    let lo = Math.min(...values), hi = Math.max(...values);
    if (lo === hi) { lo -= 1; hi += 1; }
    const span = hi - lo;
    const step = Math.pow(10, Math.floor(Math.log10(span / 5)));
    lo = Math.floor(lo / step) * step;
    hi = Math.ceil(hi  / step) * step;
    return { lo, hi };
}

function scaleX(v, lo, hi, area) { return area.x + (v - lo) / (hi - lo) * area.w; }
function scaleY(v, lo, hi, area) { return area.y + area.h - (v - lo) / (hi - lo) * area.h; }

function drawAxes(ctx, area, xRange, yRange, xLabel, yLabel, title, gridColor) {
    const { x, y, w, h } = area;
    // Grid + axis ticks
    ctx.strokeStyle = gridColor || '#e2e8f0';
    ctx.lineWidth = 1;
    const xTicks = 6, yTicks = 5;
    ctx.font = '11px system-ui, sans-serif';
    ctx.fillStyle = '#64748b';
    ctx.textAlign = 'center';
    for (let i = 0; i <= xTicks; i++) {
        const v = xRange.lo + (xRange.hi - xRange.lo) * i / xTicks;
        const px = scaleX(v, xRange.lo, xRange.hi, area);
        ctx.beginPath(); ctx.moveTo(px, y); ctx.lineTo(px, y + h); ctx.stroke();
        ctx.fillText(v.toPrecision(3), px, y + h + 16);
    }
    ctx.textAlign = 'right';
    for (let i = 0; i <= yTicks; i++) {
        const v = yRange.lo + (yRange.hi - yRange.lo) * i / yTicks;
        const py = scaleY(v, yRange.lo, yRange.hi, area);
        ctx.beginPath(); ctx.moveTo(x, py); ctx.lineTo(x + w, py); ctx.stroke();
        ctx.fillText(v.toPrecision(3), x - 6, py + 4);
    }
    // Axis border
    ctx.strokeStyle = '#94a3b8';
    ctx.lineWidth = 1.5;
    ctx.strokeRect(x, y, w, h);
    // Labels
    ctx.fillStyle = '#374151';
    ctx.font = 'bold 12px system-ui, sans-serif';
    ctx.textAlign = 'center';
    if (xLabel) ctx.fillText(xLabel, x + w / 2, y + h + 38);
    if (yLabel) {
        ctx.save();
        ctx.translate(16, y + h / 2);
        ctx.rotate(-Math.PI / 2);
        ctx.fillText(yLabel, 0, 0);
        ctx.restore();
    }
    // Title
    if (title) {
        ctx.font = 'bold 14px system-ui, sans-serif';
        ctx.fillStyle = '#1e293b';
        ctx.fillText(title, x + w / 2, y - 16);
    }
}

function drawLegend(ctx, series, x, y) {
    ctx.font = '11px system-ui, sans-serif';
    let lx = x;
    for (const s of series) {
        ctx.fillStyle = cssColor(s.color);
        ctx.fillRect(lx, y, 14, 10);
        ctx.fillStyle = '#374151';
        ctx.textAlign = 'left';
        ctx.fillText(s.label || '', lx + 18, y + 10);
        lx += ctx.measureText(s.label || '').width + 40;
    }
}

// ── Graph ─────────────────────────────────────────────────────────────────────
export class Graph {
    #width; #height; #type;
    #title = ''; #xLabel = ''; #yLabel = '';
    #series = [];

    constructor(width = 800, height = 600, type = GraphType.Line) {
        this.#width = width; this.#height = height; this.#type = type;
    }
    set_title(t)   { this.#title  = t; return this; }
    set_x_label(l) { this.#xLabel = l; return this; }
    set_y_label(l) { this.#yLabel = l; return this; }
    set_type(t)    { this.#type   = t; return this; }
    add_series(s)  { this.#series.push(s); return this; }

    /** Render to an existing HTMLCanvasElement, resizing it to (width, height). */
    render_to_canvas(canvas) {
        canvas.width  = this.#width;
        canvas.height = this.#height;
        const ctx = canvas.getContext('2d');
        ctx.clearRect(0, 0, this.#width, this.#height);
        ctx.fillStyle = '#ffffff';
        ctx.fillRect(0, 0, this.#width, this.#height);

        if (!this.#series.length) return;

        const area = plotArea(this.#width, this.#height);
        const allX = this.#series.flatMap(s => s.x_values);
        const allY = this.#series.flatMap(s => s.y_values);
        const xRange = niceRange(allX);
        const yRange = niceRange(allY);

        drawAxes(ctx, area, xRange, yRange, this.#xLabel, this.#yLabel, this.#title);

        for (let i = 0; i < this.#series.length; i++) {
            const s = this.#series[i];
            if (this.#type === GraphType.Line)    this.#drawLine   (ctx, s, area, xRange, yRange);
            if (this.#type === GraphType.Bar)     this.#drawBar    (ctx, s, area, xRange, yRange, i, this.#series.length);
            if (this.#type === GraphType.Scatter) this.#drawScatter(ctx, s, area, xRange, yRange);
        }

        drawLegend(ctx, this.#series, area.x + 4, area.y + area.h + 52 - 18);
    }

    #drawLine(ctx, s, area, xRange, yRange) {
        ctx.beginPath();
        ctx.strokeStyle = cssColor(s.color);
        ctx.lineWidth = 2.5;
        ctx.lineJoin = 'round';
        for (let i = 0; i < s.x_values.length; i++) {
            const px = scaleX(s.x_values[i], xRange.lo, xRange.hi, area);
            const py = scaleY(s.y_values[i], yRange.lo, yRange.hi, area);
            i === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
        }
        ctx.stroke();
        // Dots
        for (let i = 0; i < s.x_values.length; i++) {
            ctx.beginPath();
            ctx.arc(scaleX(s.x_values[i], xRange.lo, xRange.hi, area),
                    scaleY(s.y_values[i], yRange.lo, yRange.hi, area), 4, 0, 2 * Math.PI);
            ctx.fillStyle = cssColor(s.color);
            ctx.fill();
        }
    }

    #drawBar(ctx, s, area, xRange, yRange, idx, total) {
        const n = s.x_values.length;
        const slotW = area.w / (n || 1);
        const barW  = Math.max(4, (slotW * 0.7) / total);
        const offset = (idx - (total - 1) / 2) * barW;
        const y0 = scaleY(0, yRange.lo, yRange.hi, area);
        ctx.fillStyle = cssColor(s.color);
        for (let i = 0; i < n; i++) {
            const cx = scaleX(s.x_values[i], xRange.lo, xRange.hi, area) + offset;
            const cy = scaleY(s.y_values[i], yRange.lo, yRange.hi, area);
            ctx.fillRect(cx - barW / 2, cy, barW, y0 - cy);
        }
    }

    #drawScatter(ctx, s, area, xRange, yRange) {
        ctx.fillStyle = cssColor({ ...s.color, a: 180 });
        for (let i = 0; i < s.x_values.length; i++) {
            ctx.beginPath();
            ctx.arc(scaleX(s.x_values[i], xRange.lo, xRange.hi, area),
                    scaleY(s.y_values[i], yRange.lo, yRange.hi, area), 5, 0, 2 * Math.PI);
            ctx.fill();
        }
    }
}

// ── Table ─────────────────────────────────────────────────────────────────────
export class Table {
    #headers = [];
    #rows = [];
    #cell_padding = 8;
    #font_scale = 1;
    #header_color  = rgb(70, 130, 200);
    #border_color  = Colors.Gray;
    #alt_row_color = rgb(245, 245, 255);

    set_headers(h)          { this.#headers = h; return this; }
    add_row(r)              { this.#rows.push(r); return this; }
    set_cell_padding(p)     { this.#cell_padding = p; return this; }
    set_font_scale(s)       { this.#font_scale = s; return this; }
    set_header_color(c)     { this.#header_color = c; return this; }
    set_border_color(c)     { this.#border_color = c; return this; }
    set_alternate_row_color(c) { this.#alt_row_color = c; return this; }

    render_to_element(container) {
        container.innerHTML = '';
        const table = document.createElement('table');
        table.style.cssText = 'border-collapse:collapse;width:100%;font-family:system-ui,sans-serif;font-size:13px;';
        if (this.#headers.length) {
            const thead = table.createTHead();
            const hr = thead.insertRow();
            for (const h of this.#headers) {
                const th = document.createElement('th');
                th.textContent = h;
                th.style.cssText = `background:${cssColor(this.#header_color)};color:#fff;padding:${this.#cell_padding}px;border:1px solid ${cssColor(this.#border_color)};`;
                hr.appendChild(th);
            }
        }
        const tbody = table.createTBody();
        for (let ri = 0; ri < this.#rows.length; ri++) {
            const tr = tbody.insertRow();
            if (ri % 2 === 1) tr.style.background = cssColor(this.#alt_row_color);
            for (const cell of this.#rows[ri]) {
                const td = tr.insertCell();
                td.textContent = cell;
                td.style.cssText = `padding:${this.#cell_padding}px;border:1px solid ${cssColor(this.#border_color)};`;
            }
        }
        container.appendChild(table);
    }
}

// ── FunctionPlot ──────────────────────────────────────────────────────────────
const _EVAL_FNS = {
    'x':      x => x,
    'sin(x)': x => Math.sin(x),
    'cos(x)': x => Math.cos(x),
    'tan(x)': x => Math.tan(x),
    'x^2':    x => x*x,
    'x^3':    x => x*x*x,
    'exp(x)': x => Math.exp(x),
    'log(x)': x => Math.log(x),
    'abs(x)': x => Math.abs(x),
    'sqrt(x)': x => Math.sqrt(x),
};
function evalExpr(expr, varName, val) {
    const key = expr.replace(new RegExp(varName, 'g'), varName);
    if (_EVAL_FNS[key]) return _EVAL_FNS[key](val);
    // fallback: simple substitution eval (safe — only math chars)
    if (/^[0-9x\s+\-*/^().sincotagbqrlep]+$/i.test(expr)) {
        try { return Function(`"use strict";const ${varName}=${val};return ${expr.replace(/\^/g,'**')}`)(); }
        catch { return 0; }
    }
    return 0;
}

export class FunctionPlot {
    #w; #h; #expr = 'sin(x)'; #xMin = -10; #xMax = 10; #samples = 500; #color = Colors.Blue;
    constructor(w = 600, h = 400) { this.#w = w; this.#h = h; }
    set_equation(e) { this.#expr = e; return this; }
    set_range(a, b) { this.#xMin = a; this.#xMax = b; return this; }
    set_samples(n)  { this.#samples = n; return this; }
    set_color(c)    { this.#color = c; return this; }
    render_to_canvas(canvas) {
        canvas.width = this.#w; canvas.height = this.#h;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, this.#w, this.#h);
        const xs = [], ys = [];
        for (let i = 0; i < this.#samples; i++) {
            const x = this.#xMin + (this.#xMax - this.#xMin) * i / (this.#samples - 1);
            xs.push(x); ys.push(evalExpr(this.#expr, 'x', x));
        }
        const xRange = { lo: this.#xMin, hi: this.#xMax };
        const yRange = niceRange(ys);
        const area = plotArea(this.#w, this.#h);
        drawAxes(ctx, area, xRange, yRange, 'x', 'y', `y = ${this.#expr}`);
        ctx.beginPath();
        ctx.strokeStyle = cssColor(this.#color); ctx.lineWidth = 2.5; ctx.lineJoin = 'round';
        for (let i = 0; i < xs.length; i++) {
            const px = scaleX(xs[i], xRange.lo, xRange.hi, area);
            const py = scaleY(ys[i], yRange.lo, yRange.hi, area);
            i === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
        }
        ctx.stroke();
    }
}

export class ParametricPlot {
    #w; #h; #xExpr = 'cos(t)'; #yExpr = 'sin(t)';
    #tMin = 0; #tMax = 2 * Math.PI; #samples = 500; #color = Colors.Red;
    constructor(w = 600, h = 600) { this.#w = w; this.#h = h; }
    set_equations(xe, ye) { this.#xExpr = xe; this.#yExpr = ye; return this; }
    set_t_range(a, b)     { this.#tMin = a; this.#tMax = b; return this; }
    set_samples(n)        { this.#samples = n; return this; }
    set_color(c)          { this.#color = c; return this; }
    render_to_canvas(canvas) {
        canvas.width = this.#w; canvas.height = this.#h;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, this.#w, this.#h);
        const xs = [], ys = [];
        for (let i = 0; i < this.#samples; i++) {
            const t = this.#tMin + (this.#tMax - this.#tMin) * i / (this.#samples - 1);
            xs.push(evalExpr(this.#xExpr, 't', t));
            ys.push(evalExpr(this.#yExpr, 't', t));
        }
        const xRange = niceRange(xs), yRange = niceRange(ys);
        const area = plotArea(this.#w, this.#h);
        drawAxes(ctx, area, xRange, yRange, 'x', 'y', `x=${this.#xExpr}, y=${this.#yExpr}`);
        ctx.beginPath();
        ctx.strokeStyle = cssColor(this.#color); ctx.lineWidth = 2; ctx.lineJoin = 'round';
        for (let i = 0; i < xs.length; i++) {
            const px = scaleX(xs[i], xRange.lo, xRange.hi, area);
            const py = scaleY(ys[i], yRange.lo, yRange.hi, area);
            i === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
        }
        ctx.stroke();
    }
}

export class PolarPlot {
    #w; #h; #expr = '1 + sin(theta)'; #tMin = 0; #tMax = 2*Math.PI; #samples = 500; #color = Colors.Purple;
    constructor(w = 600, h = 600) { this.#w = w; this.#h = h; }
    set_equation(e)         { this.#expr = e; return this; }
    set_theta_range(a, b)   { this.#tMin = a; this.#tMax = b; return this; }
    set_samples(n)          { this.#samples = n; return this; }
    set_color(c)            { this.#color = c; return this; }
    render_to_canvas(canvas) {
        canvas.width = this.#w; canvas.height = this.#h;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, this.#w, this.#h);
        const xs = [], ys = [];
        for (let i = 0; i < this.#samples; i++) {
            const theta = this.#tMin + (this.#tMax - this.#tMin) * i / (this.#samples - 1);
            const r = evalExpr(this.#expr.replace(/theta/g, 't'), 't', theta);
            xs.push(r * Math.cos(theta)); ys.push(r * Math.sin(theta));
        }
        const xRange = niceRange(xs), yRange = niceRange(ys);
        const area = plotArea(this.#w, this.#h);
        drawAxes(ctx, area, xRange, yRange, '', '', `r = ${this.#expr}`);
        ctx.beginPath();
        ctx.strokeStyle = cssColor(this.#color); ctx.lineWidth = 2; ctx.lineJoin = 'round';
        for (let i = 0; i < xs.length; i++) {
            const px = scaleX(xs[i], xRange.lo, xRange.hi, area);
            const py = scaleY(ys[i], yRange.lo, yRange.hi, area);
            i === 0 ? ctx.moveTo(px, py) : ctx.lineTo(px, py);
        }
        ctx.stroke();
    }
}

export class HistogramPlot {
    #w; #h; #data = []; #bins = 20; #color = Colors.Orange;
    constructor(w = 600, h = 400) { this.#w = w; this.#h = h; }
    set_data(d)  { this.#data = d; return this; }
    set_bins(n)  { this.#bins = n; return this; }
    set_color(c) { this.#color = c; return this; }
    render_to_canvas(canvas) {
        canvas.width = this.#w; canvas.height = this.#h;
        const ctx = canvas.getContext('2d');
        ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, this.#w, this.#h);
        if (!this.#data.length) return;
        const lo = Math.min(...this.#data), hi = Math.max(...this.#data);
        const span = hi - lo || 1;
        const counts = Array(this.#bins).fill(0);
        for (const v of this.#data) counts[Math.min(this.#bins - 1, Math.floor((v - lo) / span * this.#bins))]++;
        const maxC = Math.max(...counts);
        const xRange = { lo, hi };
        const yRange = niceRange([0, maxC]);
        const area = plotArea(this.#w, this.#h);
        drawAxes(ctx, area, xRange, yRange, 'value', 'count', 'Histogram');
        ctx.fillStyle = cssColor(this.#color);
        const barW = area.w / this.#bins;
        const y0 = scaleY(0, yRange.lo, yRange.hi, area);
        for (let i = 0; i < this.#bins; i++) {
            const bx = area.x + i * barW;
            const by = scaleY(counts[i], yRange.lo, yRange.hi, area);
            ctx.fillRect(bx + 1, by, barW - 2, y0 - by);
        }
    }
}

// ── <graphics-chart> custom element ──────────────────────────────────────────
/**
 * Attributes:
 *   type        — "line" | "bar" | "scatter" | "function" | "parametric" | "polar" | "histogram"
 *   title       — chart title string
 *   x-label     — x axis label
 *   y-label     — y axis label
 *   width       — canvas width  (default 600)
 *   height      — canvas height (default 350)
 *   series      — JSON array of { label, x, y, color? } objects (for line/bar/scatter)
 *   expr        — expression string (for function/polar)
 *   x-expr      — x expression  (for parametric)
 *   y-expr      — y expression  (for parametric)
 *   data        — JSON number array (for histogram)
 *   bins        — bin count     (for histogram)
 */
class GraphicsChart extends HTMLElement {
    static get observedAttributes() {
        return ['type','title','x-label','y-label','width','height',
                'series','expr','x-expr','y-expr','data','bins'];
    }
    connectedCallback() { this._render(); }
    attributeChangedCallback() { if (this.isConnected) this._render(); }

    _render() {
        if (!this._canvas) {
            this._canvas = document.createElement('canvas');
            this._canvas.style.cssText = 'display:block;max-width:100%;';
            this.innerHTML = '';
            this.appendChild(this._canvas);
        }
        const type   = (this.getAttribute('type') || 'line').toLowerCase();
        const W      = parseInt(this.getAttribute('width')  || '600', 10);
        const H      = parseInt(this.getAttribute('height') || '350', 10);
        const title  = this.getAttribute('title')   || '';
        const xLabel = this.getAttribute('x-label') || '';
        const yLabel = this.getAttribute('y-label') || '';

        if (type === 'function') {
            const fp = new FunctionPlot(W, H);
            fp.set_equation(this.getAttribute('expr') || 'sin(x)');
            const range = this.getAttribute('range');
            if (range) { const [a,b] = range.split(',').map(Number); fp.set_range(a, b); }
            fp.render_to_canvas(this._canvas);
            return;
        }
        if (type === 'parametric') {
            const pp = new ParametricPlot(W, H);
            pp.set_equations(this.getAttribute('x-expr') || 'cos(t)', this.getAttribute('y-expr') || 'sin(t)');
            pp.render_to_canvas(this._canvas);
            return;
        }
        if (type === 'polar') {
            const pol = new PolarPlot(W, H);
            pol.set_equation(this.getAttribute('expr') || '1 + sin(theta)');
            pol.render_to_canvas(this._canvas);
            return;
        }
        if (type === 'histogram') {
            const hp = new HistogramPlot(W, H);
            try { hp.set_data(JSON.parse(this.getAttribute('data') || '[]')); } catch {}
            const bins = parseInt(this.getAttribute('bins') || '20', 10);
            hp.set_bins(bins);
            hp.render_to_canvas(this._canvas);
            return;
        }

        // line | bar | scatter
        const typeMap = { line: GraphType.Line, bar: GraphType.Bar, scatter: GraphType.Scatter };
        const g = new Graph(W, H, typeMap[type] || GraphType.Line);
        g.set_title(title).set_x_label(xLabel).set_y_label(yLabel);

        let seriesArr = [];
        try { seriesArr = JSON.parse(this.getAttribute('series') || '[]'); } catch {}
        const palette = [Colors.Blue, Colors.Red, Colors.Green, Colors.Orange, Colors.Purple, Colors.Cyan];
        seriesArr.forEach((s, i) => {
            let color = palette[i % palette.length];
            if (s.color) {
                if (Colors[s.color]) color = Colors[s.color];
                else if (Array.isArray(s.color)) color = rgb(...s.color);
            }
            g.add_series({ label: s.label || '', x_values: s.x || s.x_values || [], y_values: s.y || s.y_values || [], color });
        });
        g.render_to_canvas(this._canvas);
    }
}
customElements.define('graphics-chart', GraphicsChart);
