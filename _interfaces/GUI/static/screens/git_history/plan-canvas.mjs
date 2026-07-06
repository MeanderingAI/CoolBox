// Custom element <plan-canvas>
// Canvas-based interactive browser for the plan/ directory tree.
// Pan:  drag on canvas.   Zoom: mouse wheel.   Click: open file.

// ── Colour palette ────────────────────────────────────────────────────────────
const P = {
    canvasBg:    '#12131f',
    boardBg:     '#1e1f35',
    boardBorder: '#2e3055',
    headerBg:    '#0e639c',
    headerBg2:   '#0a4d7a',
    chip:        '#171828',
    chipHover:   '#1a4f72',
    chipBorder:  '#2a3a5e',
    chipText:    '#c9d1e3',
    headText:    '#ffffff',
    parentText:  '#7dd3fc',
    dotGrid:     '#1c1d30',
};

// ── Layout constants ──────────────────────────────────────────────────────────
const BW        = 276;   // board width
const HEAD_H    = 54;    // board header height
const PAD       = 14;    // board inner padding
const CHIP_H    = 28;    // file chip height
const CHIP_GAP  = 6;     // gap between chips
const BOARD_MX  = 44;   // horizontal margin between boards
const BOARD_MY  = 52;   // vertical margin between board rows
const COLS      = 3;     // boards per row
const CORNER    = 10;    // corner radius
const CHIP_X    = 10;    // chip x inset inside board
const CHIP_W    = BW - CHIP_X * 2;

function boardH(files) {
    return HEAD_H + PAD + files.length * (CHIP_H + CHIP_GAP) - CHIP_GAP + PAD;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function rr(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h - r);
    ctx.quadraticCurveTo(x + w, y + h, x + w - r, y + h);
    ctx.lineTo(x + r, y + h);
    ctx.quadraticCurveTo(x, y + h, x, y + h - r);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
}

function rrTop(ctx, x, y, w, h, r) {
    ctx.beginPath();
    ctx.moveTo(x + r, y);
    ctx.lineTo(x + w - r, y);
    ctx.quadraticCurveTo(x + w, y, x + w, y + r);
    ctx.lineTo(x + w, y + h);
    ctx.lineTo(x, y + h);
    ctx.lineTo(x, y + r);
    ctx.quadraticCurveTo(x, y, x + r, y);
    ctx.closePath();
}

function truncate(ctx, text, maxW) {
    if (ctx.measureText(text).width <= maxW) return text;
    let s = text;
    while (s.length > 1 && ctx.measureText(s + '…').width > maxW) s = s.slice(0, -1);
    return s + '…';
}

function escHtml(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

// ── Tiny markdown → HTML renderer ────────────────────────────────────────────
function renderMd(md) {
    const lines = md.split('\n');
    const out = [];
    let inCode = false;
    let codeLang = '';
    let codeBuf = [];
    let inList = false;

    const flushList = () => {
        if (inList) { out.push('</ul>'); inList = false; }
    };

    for (let i = 0; i < lines.length; i++) {
        let l = lines[i];

        // Fenced code block
        if (l.startsWith('```')) {
            if (!inCode) {
                flushList();
                inCode = true;
                codeLang = l.slice(3).trim();
                codeBuf = [];
            } else {
                out.push(`<pre><code class="lang-${escHtml(codeLang)}">${escHtml(codeBuf.join('\n'))}</code></pre>`);
                inCode = false;
            }
            continue;
        }
        if (inCode) { codeBuf.push(l); continue; }

        // Table row
        if (l.startsWith('|')) {
            flushList();
            // Separator row
            if (/^\|[-\s:|]+\|/.test(l)) continue;
            const cells = l.split('|').filter((_, i, a) => i > 0 && i < a.length - 1);
            // Check if previous non-empty line was a table header (we detect by checking out stack)
            const isHead = out.length > 0 && out[out.length - 1].startsWith('<thead>');
            if (!out.some(o => o === '<table>')) out.push('<table>');
            if (!isHead && !out.some(o => o.startsWith('<tbody'))) {
                // first row is header
                out.push('<thead><tr>' + cells.map(c => `<th>${inline(c.trim())}</th>`).join('') + '</tr>');
            } else {
                if (!out.some(o => o === '<tbody>')) out.push('<tbody>');
                out.push('<tr>' + cells.map(c => `<td>${inline(c.trim())}</td>`).join('') + '</tr>');
            }
            continue;
        } else if (out.some(o => o === '<table>')) {
            if (out.some(o => o === '<tbody>')) out.push('</tbody>');
            if (out.some(o => o.startsWith('<thead>'))) out.push('</thead>');
            out.push('</table>');
            // remove table markers
            const clean = [];
            let tOpen = false;
            for (const o of out) {
                if (o === '<table>') { tOpen = true; clean.push(o); continue; }
                if (!tOpen || (o !== '<tbody>' && o !== '</tbody>' && !o.startsWith('<thead>'))) clean.push(o);
                else clean.push(o);
            }
        }

        // Headings
        if (l.startsWith('# '))   { flushList(); out.push(`<h1>${inline(l.slice(2))}</h1>`); continue; }
        if (l.startsWith('## '))  { flushList(); out.push(`<h2>${inline(l.slice(3))}</h2>`); continue; }
        if (l.startsWith('### ')) { flushList(); out.push(`<h3>${inline(l.slice(4))}</h3>`); continue; }

        // HR
        if (/^---+$/.test(l.trim())) { flushList(); out.push('<hr>'); continue; }

        // Bullet list
        if (/^[\-\*] /.test(l)) {
            if (!inList) { out.push('<ul>'); inList = true; }
            out.push(`<li>${inline(l.slice(2))}</li>`);
            continue;
        }

        // Blank line
        if (l.trim() === '') {
            flushList();
            out.push('<br>');
            continue;
        }

        flushList();
        out.push(`<p>${inline(l)}</p>`);
    }
    flushList();
    if (inCode) out.push(`<pre><code>${escHtml(codeBuf.join('\n'))}</code></pre>`);
    return out.join('\n');
}

function inline(s) {
    return s
        .replace(/`([^`]+)`/g, '<code>$1</code>')
        .replace(/\*\*(.+?)\*\*/g, '<strong>$1</strong>')
        .replace(/\*(.+?)\*/g,   '<em>$1</em>');
}

// ── Tree → boards ─────────────────────────────────────────────────────────────
function flattenBoards(node, parent = '') {
    const boards = [];
    function walk(n, par) {
        const files = (n.children || []).filter(c => c.type === 'file');
        const dirs  = (n.children || []).filter(c => c.type === 'dir');
        if (files.length) boards.push({ name: n.name, path: n.path, parent: par, files });
        dirs.forEach(d => walk(d, n.name));
    }
    (node.children || []).forEach(c => walk(c, node.name));
    return boards;
}

function layoutBoards(boards) {
    const rows = [];
    for (let i = 0; i < boards.length; i += COLS) rows.push(boards.slice(i, i + COLS));

    const laid = [];
    let y = PAD * 2;
    for (const row of rows) {
        const rowH = Math.max(...row.map(b => boardH(b.files)));
        row.forEach((b, col) => {
            const x = col * (BW + BOARD_MX) + PAD * 2;
            laid.push({ ...b, x, y });
        });
        y += rowH + BOARD_MY;
    }
    return laid;
}

// ── Hit rects ─────────────────────────────────────────────────────────────────
function computeHits(boards) {
    const hits = [];
    for (const b of boards) {
        b.files.forEach((file, fi) => {
            const cy = b.y + HEAD_H + PAD + fi * (CHIP_H + CHIP_GAP);
            hits.push({ x: b.x + CHIP_X, y: cy, w: CHIP_W, h: CHIP_H, file });
        });
    }
    return hits;
}

// ── Shadow DOM styles ─────────────────────────────────────────────────────────
const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    height: calc(100vh - 175px);
    font-family: 'Segoe UI', Arial, sans-serif;
    overflow: hidden;
}

/* ── Toolbar ─────────────────────────────────────────── */
.toolbar {
    display: flex;
    align-items: center;
    gap: 0.6em;
    padding: 0.55em 1em;
    background: #f5f6fa;
    border-bottom: 1px solid #dde1ea;
    flex-shrink: 0;
}

.tb-label {
    font-size: 0.78em;
    font-weight: 700;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: #6b7280;
}

.tb-btn {
    padding: 0.3em 0.8em;
    border: 1px solid #0e639c;
    border-radius: 5px;
    background: transparent;
    color: #0e639c;
    font-size: 0.8em;
    cursor: pointer;
    transition: background 0.12s, color 0.12s;
}
.tb-btn:hover { background: #0e639c; color: #fff; }

.tb-select {
    padding: 0.3em 0.6em;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    background: #fff;
    font-size: 0.8em;
    color: #374151;
    cursor: pointer;
}

.tb-hint {
    margin-left: auto;
    font-size: 0.75em;
    color: #9ca3af;
}

/* ── Body ─────────────────────────────────────────────── */
.body {
    display: flex;
    flex: 1;
    overflow: hidden;
}

/* ── Canvas pane ──────────────────────────────────────── */
.canvas-wrap {
    flex: 1;
    overflow: hidden;
    position: relative;
    cursor: grab;
}
.canvas-wrap.dragging { cursor: grabbing; }

canvas {
    display: block;
    width: 100%;
    height: 100%;
    image-rendering: pixelated;
}

/* ── Content panel ────────────────────────────────────── */
.content-panel {
    width: 0;
    flex-shrink: 0;
    overflow: hidden;
    background: #fff;
    border-left: 2px solid #e5e7eb;
    display: flex;
    flex-direction: column;
    transition: width 0.22s ease;
}
.content-panel.open { width: 38%; }

.content-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0.7em 1em;
    background: #0e639c;
    color: #fff;
    font-size: 0.85em;
    font-weight: 600;
    flex-shrink: 0;
}

.content-close {
    background: none;
    border: none;
    color: #fff;
    font-size: 1.1em;
    cursor: pointer;
    line-height: 1;
    opacity: 0.8;
    transition: opacity 0.12s;
}
.content-close:hover { opacity: 1; }

.content-body {
    flex: 1;
    overflow-y: auto;
    padding: 1.2em 1.4em;
    font-size: 0.85em;
    line-height: 1.65;
    color: #1f2937;
}

/* ── Rendered markdown ────────────────────────────────── */
.content-body h1 { font-size: 1.3em; color: #0e639c; margin: 0.6em 0 0.4em; border-bottom: 1px solid #e5e7eb; padding-bottom: 0.25em; }
.content-body h2 { font-size: 1.1em; color: #1e40af; margin: 1em 0 0.35em; }
.content-body h3 { font-size: 0.95em; color: #374151; margin: 0.9em 0 0.3em; }
.content-body p  { margin: 0.3em 0; }
.content-body hr { border: none; border-top: 1px solid #e5e7eb; margin: 1em 0; }
.content-body ul { padding-left: 1.4em; margin: 0.4em 0; }
.content-body li { margin: 0.15em 0; }
.content-body code {
    background: #f3f4f6;
    border: 1px solid #e5e7eb;
    border-radius: 3px;
    padding: 0.1em 0.35em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.9em;
}
.content-body pre {
    background: #1e1e2e;
    color: #cdd6f4;
    border-radius: 6px;
    padding: 0.8em 1em;
    overflow-x: auto;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.82em;
    line-height: 1.5;
    margin: 0.6em 0;
}
.content-body pre code {
    background: none;
    border: none;
    padding: 0;
    font-size: 1em;
}
.content-body table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.88em;
    margin: 0.6em 0;
}
.content-body th {
    background: #f3f4f6;
    border: 1px solid #e5e7eb;
    padding: 0.4em 0.7em;
    text-align: left;
    font-weight: 600;
    color: #374151;
}
.content-body td {
    border: 1px solid #e5e7eb;
    padding: 0.38em 0.7em;
    color: #1f2937;
}
.content-body tr:nth-child(even) td { background: #f9fafb; }

/* ── Status bar ───────────────────────────────────────── */
.status-bar {
    flex-shrink: 0;
    padding: 0.3em 1em;
    background: #1e1e2e;
    color: #7dd3fc;
    font-size: 0.72em;
    font-family: 'Cascadia Code', monospace;
    border-top: 1px solid #12131f;
}
`;

// ── Custom element ────────────────────────────────────────────────────────────
class PlanCanvas extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        // Style
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        toolbar.innerHTML = `
            <span class="tb-label">📋 Plan Browser</span>
            <select id="sel-version" class="tb-select"><option value="">All versions</option></select>
            <button class="tb-btn" id="btn-reset">Reset View</button>
            <button class="tb-btn" id="btn-refresh">↺ Refresh</button>
            <span class="tb-hint">Drag to pan &nbsp;·&nbsp; Wheel to zoom &nbsp;·&nbsp; Click file to open</span>
        `;
        shadow.appendChild(toolbar);

        // Body
        const body = document.createElement('div');
        body.className = 'body';
        shadow.appendChild(body);

        // Canvas wrap
        this._wrap = document.createElement('div');
        this._wrap.className = 'canvas-wrap';
        body.appendChild(this._wrap);

        this._canvas = document.createElement('canvas');
        this._wrap.appendChild(this._canvas);
        this._ctx = this._canvas.getContext('2d');

        // Content panel
        this._panel = document.createElement('div');
        this._panel.className = 'content-panel';
        this._panel.innerHTML = `
            <div class="content-header">
                <span id="panel-title">—</span>
                <button class="content-close" id="btn-close">✕</button>
            </div>
            <div class="content-body" id="panel-body"></div>
        `;
        body.appendChild(this._panel);

        // Status bar
        this._statusBar = document.createElement('div');
        this._statusBar.className = 'status-bar';
        this._statusBar.textContent = 'Loading plans…';
        shadow.appendChild(this._statusBar);

        // State
        this._allBoards = [];
        this._boards  = [];
        this._hits    = [];
        this._ox      = 40;   // pan offset x
        this._oy      = 40;   // pan offset y
        this._scale   = 1.0;
        this._drag    = null; // { startX, startY, ox0, oy0 }
        this._hoverPath = null;
        this._versionFilter = '';

        // Events
        this._canvas.addEventListener('mousedown', e => this._onDown(e));
        this._canvas.addEventListener('mousemove', e => this._onMove(e));
        this._canvas.addEventListener('mouseup',   e => this._onUp(e));
        this._canvas.addEventListener('mouseleave',() => this._onLeave());
        this._canvas.addEventListener('wheel',     e => this._onWheel(e), { passive: false });
        this._canvas.addEventListener('click',     e => this._onClick(e));

        shadow.getElementById('btn-close').addEventListener('click', () => this._closePanel());
        shadow.getElementById('btn-reset').addEventListener('click', () => this._resetView());
        shadow.getElementById('btn-refresh').addEventListener('click', () => this._load());
        shadow.getElementById('sel-version').addEventListener('change', e => {
            this._versionFilter = e.target.value;
            this._applyFilter();
        });

        // ResizeObserver
        this._ro = new ResizeObserver(() => this._resize());
        this._ro.observe(this._wrap);

        this._load();
    }

    disconnectedCallback() {
        this._ro?.disconnect();
    }

    // ── Data load ─────────────────────────────────────────────────────────────
    async _load() {
        this._statusBar.textContent = 'Fetching plan tree…';
        try {
            const res = await fetch('/plans');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const tree = await res.json();
            this._allBoards = layoutBoards(flattenBoards(tree));
            this._populateVersionSelect();
            this._applyFilter();
        } catch (e) {
            this._statusBar.textContent = `Error loading plans: ${e.message}`;
        }
    }

    _populateVersionSelect() {
        const sel = this.shadowRoot.getElementById('sel-version');
        const current = sel.value;
        while (sel.options.length > 1) sel.remove(1);
        const parents = [...new Set(this._allBoards.map(b => b.parent))].sort();
        for (const p of parents) {
            const opt = document.createElement('option');
            opt.value = p;
            opt.textContent = p;
            if (p === current) opt.selected = true;
            sel.appendChild(opt);
        }
        // Restore or keep current filter
        if (!parents.includes(this._versionFilter)) this._versionFilter = '';
        sel.value = this._versionFilter;
    }

    _applyFilter() {
        const f = this._versionFilter;
        const filtered = f ? this._allBoards.filter(b => b.parent === f) : this._allBoards;
        this._boards = filtered;
        this._hits   = computeHits(this._boards);
        const total  = this._boards.reduce((n, b) => n + b.files.length, 0);
        this._statusBar.textContent =
            `${this._boards.length} folder(s) · ${total} plan file(s)` +
            (f ? ` · filtered: ${f}` : '');
        this._resize();
    }

    // ── Resize ────────────────────────────────────────────────────────────────
    _resize() {
        const dpr = window.devicePixelRatio || 1;
        const w   = this._wrap.clientWidth;
        const h   = this._wrap.clientHeight;
        this._canvas.width  = w * dpr;
        this._canvas.height = h * dpr;
        this._ctx.scale(dpr, dpr);
        this._cw = w;
        this._ch = h;
        this._draw();
    }

    // ── Draw ──────────────────────────────────────────────────────────────────
    _draw() {
        const ctx = this._ctx;
        const W   = this._cw || this._canvas.width;
        const H   = this._ch || this._canvas.height;

        // Background
        ctx.fillStyle = P.canvasBg;
        ctx.fillRect(0, 0, W, H);

        // Dot grid
        this._drawGrid(ctx, W, H);

        // World transform
        ctx.save();
        ctx.translate(this._ox, this._oy);
        ctx.scale(this._scale, this._scale);

        for (const b of this._boards) this._drawBoard(ctx, b);

        ctx.restore();
    }

    _drawGrid(ctx, W, H) {
        const spacing = 30 * this._scale;
        const offX = ((this._ox % spacing) + spacing) % spacing;
        const offY = ((this._oy % spacing) + spacing) % spacing;
        ctx.fillStyle = '#1e1f35';
        for (let x = offX; x < W; x += spacing) {
            for (let y = offY; y < H; y += spacing) {
                ctx.fillRect(x - 1, y - 1, 2, 2);
            }
        }
    }

    _drawBoard(ctx, b) {
        const { x, y, name, parent, files } = b;
        const h = boardH(files);

        // Drop shadow
        ctx.save();
        ctx.shadowColor  = 'rgba(0,0,0,0.5)';
        ctx.shadowBlur   = 14;
        ctx.shadowOffsetY = 5;
        rr(ctx, x, y, BW, h, CORNER);
        ctx.fillStyle = P.boardBg;
        ctx.fill();
        ctx.restore();

        // Border
        rr(ctx, x, y, BW, h, CORNER);
        ctx.strokeStyle = P.boardBorder;
        ctx.lineWidth = 1;
        ctx.stroke();

        // Header gradient
        const grad = ctx.createLinearGradient(x, y, x, y + HEAD_H);
        grad.addColorStop(0, P.headerBg);
        grad.addColorStop(1, P.headerBg2);
        rrTop(ctx, x, y, BW, HEAD_H, CORNER);
        ctx.fillStyle = grad;
        ctx.fill();

        // Parent label
        ctx.font = '10px "Cascadia Code", monospace';
        ctx.fillStyle = P.parentText;
        ctx.fillText(parent, x + 14, y + 18);

        // Board name
        ctx.font = 'bold 15px "Cascadia Code", monospace';
        ctx.fillStyle = P.headText;
        ctx.fillText(truncate(ctx, name, BW - 28), x + 14, y + 40);

        // Chips
        files.forEach((file, fi) => {
            const cy   = y + HEAD_H + PAD + fi * (CHIP_H + CHIP_GAP);
            const isHov = this._hoverPath === file.path;

            rr(ctx, x + CHIP_X, cy, CHIP_W, CHIP_H, 5);
            ctx.fillStyle   = isHov ? P.chipHover : P.chip;
            ctx.strokeStyle = isHov ? '#1e7ac4' : P.chipBorder;
            ctx.lineWidth   = 1;
            ctx.fill();
            ctx.stroke();

            ctx.font      = '11.5px "Cascadia Code", monospace';
            ctx.fillStyle = P.chipText;
            const label   = file.name.replace(/\.md$/i, '');
            ctx.fillText(truncate(ctx, label, CHIP_W - 20), x + CHIP_X + 10, cy + 18);
        });
    }

    // ── Coordinate helpers ────────────────────────────────────────────────────
    _toWorld(clientX, clientY) {
        const rect = this._canvas.getBoundingClientRect();
        const mx   = clientX - rect.left;
        const my   = clientY - rect.top;
        return {
            wx: (mx - this._ox) / this._scale,
            wy: (my - this._oy) / this._scale,
        };
    }

    _hitTest(wx, wy) {
        for (const h of this._hits) {
            if (wx >= h.x && wx <= h.x + h.w && wy >= h.y && wy <= h.y + h.h) return h.file;
        }
        return null;
    }

    // ── Events ────────────────────────────────────────────────────────────────
    _onDown(e) {
        if (e.button !== 0) return;
        this._drag = { startX: e.clientX, startY: e.clientY, ox0: this._ox, oy0: this._oy };
        this._wrap.classList.add('dragging');
        this._moved = false;
    }

    _onMove(e) {
        if (this._drag) {
            const dx = e.clientX - this._drag.startX;
            const dy = e.clientY - this._drag.startY;
            if (Math.abs(dx) + Math.abs(dy) > 3) this._moved = true;
            this._ox = this._drag.ox0 + dx;
            this._oy = this._drag.oy0 + dy;
            this._draw();
            return;
        }
        const { wx, wy } = this._toWorld(e.clientX, e.clientY);
        const hit = this._hitTest(wx, wy);
        const path = hit ? hit.path : null;
        if (path !== this._hoverPath) {
            this._hoverPath = path;
            this._canvas.style.cursor = hit ? 'pointer' : 'grab';
            this._draw();
        }
    }

    _onUp(e) {
        this._drag = null;
        this._wrap.classList.remove('dragging');
    }

    _onLeave() {
        this._drag = null;
        this._wrap.classList.remove('dragging');
        this._hoverPath = null;
        this._draw();
    }

    _onWheel(e) {
        e.preventDefault();
        const rect   = this._canvas.getBoundingClientRect();
        const mx     = e.clientX - rect.left;
        const my     = e.clientY - rect.top;
        const factor = e.deltaY < 0 ? 1.1 : 0.9;
        const newScale = Math.max(0.25, Math.min(3, this._scale * factor));
        // Zoom toward cursor
        this._ox = mx - (mx - this._ox) * (newScale / this._scale);
        this._oy = my - (my - this._oy) * (newScale / this._scale);
        this._scale = newScale;
        this._draw();
    }

    _onClick(e) {
        if (this._moved) { this._moved = false; return; }
        const { wx, wy } = this._toWorld(e.clientX, e.clientY);
        const hit = this._hitTest(wx, wy);
        if (hit) this._openFile(hit);
    }

    // ── Reset view ────────────────────────────────────────────────────────────
    _resetView() {
        this._ox    = 40;
        this._oy    = 40;
        this._scale = 1.0;
        this._draw();
    }

    // ── Content panel ─────────────────────────────────────────────────────────
    async _openFile(file) {
        const shadow = this.shadowRoot;
        const title  = shadow.getElementById('panel-title');
        const bdy    = shadow.getElementById('panel-body');
        title.textContent = file.name;
        bdy.innerHTML = '<em style="color:#9ca3af">Loading…</em>';
        this._panel.classList.add('open');

        try {
            const res = await fetch(`/plans/content?path=${encodeURIComponent(file.path)}`);
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (data.error) throw new Error(data.error);
            bdy.innerHTML = renderMd(data.content);
        } catch (err) {
            bdy.innerHTML = `<span style="color:#ef4444">Error: ${escHtml(err.message)}</span>`;
        }
    }

    _closePanel() {
        this._panel.classList.remove('open');
    }
}

customElements.define('plan-canvas', PlanCanvas);
