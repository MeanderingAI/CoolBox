// Custom element <doc-builder>
// Trigger a Doxygen build and preview the generated HTML site in an iframe.

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Status banner ───────────────────────────────────── */
.status-bar {
    display: flex;
    align-items: center;
    gap: 0.8em;
    padding: 0.6em 1em;
    border-radius: 8px;
    margin-bottom: 1em;
    font-size: 0.88em;
    flex-wrap: wrap;
}
.status-bar.ready   { background: #f0fdf4; border: 1px solid #bbf7d0; color: #15803d; }
.status-bar.missing { background: #fefce8; border: 1px solid #fde68a; color: #92400e; }
.status-bar.error   { background: #fef2f2; border: 1px solid #fecaca; color: #991b1b; }
.status-bar.loading { background: #f1f5f9; border: 1px solid #e2e8f0; color: #64748b; }

.status-icon { font-size: 1.2em; flex-shrink: 0; }
.status-text { flex: 1; }
.status-meta { font-size: 0.82em; opacity: 0.75; white-space: nowrap; }

/* ── Actions row ─────────────────────────────────────── */
.actions {
    display: flex;
    align-items: center;
    gap: 0.6em;
    margin-bottom: 1em;
    flex-wrap: wrap;
}

.btn {
    padding: 0.4em 1.1em;
    border-radius: 6px;
    font-size: 0.85em;
    font-family: inherit;
    cursor: pointer;
    transition: background 0.15s, color 0.15s, opacity 0.15s;
    border: 1px solid transparent;
}
.btn:disabled { opacity: 0.5; cursor: not-allowed; }

.btn-primary {
    background: #0e639c;
    color: #fff;
    border-color: #0e639c;
}
.btn-primary:not(:disabled):hover { background: #0a4f7e; }

.btn-outline {
    background: transparent;
    color: #0e639c;
    border-color: #0e639c;
}
.btn-outline:not(:disabled):hover { background: #eff6ff; }

.btn-ghost {
    background: transparent;
    color: #6b7280;
    border-color: #d1d5db;
}
.btn-ghost:not(:disabled):hover { background: #f3f4f6; }

.spinner {
    display: inline-block;
    width: 0.85em;
    height: 0.85em;
    border: 2px solid currentColor;
    border-right-color: transparent;
    border-radius: 50%;
    animation: spin 0.6s linear infinite;
    vertical-align: middle;
    margin-right: 0.35em;
}
@keyframes spin { to { transform: rotate(360deg); } }

/* ── Build console ───────────────────────────────────── */
.console-wrap {
    margin-bottom: 1em;
    border-radius: 8px;
    overflow: hidden;
    border: 1px solid #e5e7eb;
}
.console-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    padding: 0.4em 0.8em;
    background: #1e1e2e;
    color: #9ca3af;
    font-size: 0.78em;
    cursor: pointer;
    user-select: none;
}
.console-header:hover { background: #2a2a3e; }
.console-chevron { transition: transform 0.15s; }
.console-header[aria-expanded="false"] .console-chevron { transform: rotate(-90deg); }

.console-body {
    background: #0d1117;
    color: #c9d1d9;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace;
    font-size: 0.78em;
    padding: 0.7em 0.9em;
    max-height: 280px;
    overflow-y: auto;
    white-space: pre-wrap;
    word-break: break-word;
}
.console-body.hidden { display: none; }
.line-warn  { color: #f59e0b; }
.line-error { color: #f97583; }
.line-ok    { color: #7ee787; }

/* ── Preview section ─────────────────────────────────── */
.preview-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 0.5em;
    flex-wrap: wrap;
    gap: 0.5em;
}
.preview-title {
    font-size: 0.88em;
    font-weight: 600;
    color: #374151;
}
.preview-empty {
    padding: 3em;
    text-align: center;
    color: #9ca3af;
    border: 2px dashed #e5e7eb;
    border-radius: 10px;
    font-size: 0.9em;
}

/* ── Height control ──────────────────────────────────── */
.height-select {
    padding: 0.25em 0.5em;
    border: 1px solid #d1d5db;
    border-radius: 5px;
    font-size: 0.8em;
    background: #f9fafb;
    cursor: pointer;
}

/* ── iframe ──────────────────────────────────────────── */
.preview-frame {
    width: 100%;
    border: 1px solid #e5e7eb;
    border-radius: 10px;
    background: #fff;
    display: block;
    transition: height 0.2s;
}
`;

function _esc(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
}

// Colour-code Doxygen output lines
function _annotateLines(text) {
    return text.split('\n').map(line => {
        const lo = line.toLowerCase();
        if (lo.includes('error') || lo.startsWith('error'))
            return `<span class="line-error">${_esc(line)}</span>`;
        if (lo.includes('warning') || lo.startsWith('warning'))
            return `<span class="line-warn">${_esc(line)}</span>`;
        if (lo.includes('generating') || lo.includes('finished'))
            return `<span class="line-ok">${_esc(line)}</span>`;
        return _esc(line);
    }).join('\n');
}

const HEIGHTS = ['400px', '550px', '700px', '900px', '100vh'];

class DocBuilder extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Status banner
        this._statusBar = document.createElement('div');
        this._statusBar.className = 'status-bar loading';
        this._statusBar.innerHTML = `<span class="status-icon">⏳</span><span class="status-text">Checking documentation status…</span>`;
        shadow.appendChild(this._statusBar);

        // Actions row
        const actions = document.createElement('div');
        actions.className = 'actions';

        this._buildBtn = document.createElement('button');
        this._buildBtn.className = 'btn btn-primary';
        this._buildBtn.textContent = '▶ Build Docs';
        this._buildBtn.addEventListener('click', () => this._build());

        this._previewBtn = document.createElement('button');
        this._previewBtn.className = 'btn btn-outline';
        this._previewBtn.textContent = '🖼 Refresh Preview';
        this._previewBtn.disabled = true;
        this._previewBtn.addEventListener('click', () => this._refreshPreview());

        const openBtn = document.createElement('a');
        openBtn.className = 'btn btn-ghost';
        openBtn.textContent = '↗ Open in new tab';
        openBtn.href = '/docs-preview/index.html';
        openBtn.target = '_blank';
        openBtn.rel = 'noopener noreferrer';
        openBtn.style.textDecoration = 'none';
        this._openBtn = openBtn;

        actions.appendChild(this._buildBtn);
        actions.appendChild(this._previewBtn);
        actions.appendChild(openBtn);
        shadow.appendChild(actions);

        // Build console
        this._consoleWrap = document.createElement('div');
        this._consoleWrap.className = 'console-wrap';
        this._consoleWrap.style.display = 'none';

        const consoleHeader = document.createElement('div');
        consoleHeader.className = 'console-header';
        consoleHeader.setAttribute('aria-expanded', 'true');
        consoleHeader.innerHTML = `<span>Build Output</span><span class="console-chevron">▾</span>`;
        consoleHeader.addEventListener('click', () => {
            const expanded = consoleHeader.getAttribute('aria-expanded') === 'true';
            consoleHeader.setAttribute('aria-expanded', expanded ? 'false' : 'true');
            this._consoleBody.classList.toggle('hidden', expanded);
        });

        this._consoleBody = document.createElement('div');
        this._consoleBody.className = 'console-body';

        this._consoleWrap.appendChild(consoleHeader);
        this._consoleWrap.appendChild(this._consoleBody);
        shadow.appendChild(this._consoleWrap);

        // Preview section
        const previewHeader = document.createElement('div');
        previewHeader.className = 'preview-header';

        const previewTitle = document.createElement('span');
        previewTitle.className = 'preview-title';
        previewTitle.textContent = 'Documentation Preview';

        const heightSelect = document.createElement('select');
        heightSelect.className = 'height-select';
        heightSelect.title = 'Preview height';
        HEIGHTS.forEach(h => {
            const opt = document.createElement('option');
            opt.value = h;
            opt.textContent = h;
            if (h === '550px') opt.selected = true;
            heightSelect.appendChild(opt);
        });
        heightSelect.addEventListener('change', () => {
            if (this._frame) this._frame.style.height = heightSelect.value;
        });

        previewHeader.appendChild(previewTitle);
        previewHeader.appendChild(heightSelect);
        shadow.appendChild(previewHeader);

        this._previewContainer = document.createElement('div');
        shadow.appendChild(this._previewContainer);

        this._shadow = shadow;
        this._heightSelect = heightSelect;

        this._checkStatus();
    }

    async _checkStatus() {
        try {
            const res = await fetch('/docs/status');
            const data = await res.json();
            this._updateStatusBar(data);
            if (data.exists) {
                this._previewBtn.disabled = false;
                this._showPreview();
            } else {
                this._showEmpty();
            }
        } catch (err) {
            this._statusBar.className = 'status-bar error';
            this._statusBar.innerHTML = `<span class="status-icon">⚠</span><span class="status-text">Could not reach server: ${_esc(String(err))}</span>`;
        }
    }

    _updateStatusBar(data) {
        if (data.exists) {
            this._statusBar.className = 'status-bar ready';
            this._statusBar.innerHTML = `
                <span class="status-icon">✅</span>
                <span class="status-text">Documentation is built — <strong>${data.file_count}</strong> pages</span>
                <span class="status-meta">Last built: ${_esc(data.last_built || 'unknown')}</span>`;
        } else {
            this._statusBar.className = 'status-bar missing';
            this._statusBar.innerHTML = `
                <span class="status-icon">📭</span>
                <span class="status-text">No documentation found. Click <em>Build Docs</em> to generate with Doxygen.</span>`;
        }
    }

    async _build() {
        this._buildBtn.disabled = true;
        this._buildBtn.innerHTML = `<span class="spinner"></span>Building…`;
        this._consoleWrap.style.display = '';
        this._consoleBody.innerHTML = '<span style="color:#79c0ff">Running: doxygen Doxyfile …</span>\n';

        this._statusBar.className = 'status-bar loading';
        this._statusBar.innerHTML = `<span class="status-icon"><span class="spinner"></span></span><span class="status-text">Building documentation…</span>`;

        try {
            const res = await fetch('/docs/build', { method: 'POST' });
            const data = await res.json();

            this._consoleBody.innerHTML = _annotateLines(data.output || '(no output)');
            // Scroll to bottom
            this._consoleBody.scrollTop = this._consoleBody.scrollHeight;

            if (data.success) {
                // Re-check status to update banner + page count
                const statusRes = await fetch('/docs/status');
                const statusData = await statusRes.json();
                this._updateStatusBar(statusData);
                this._previewBtn.disabled = false;
                this._showPreview();
            } else {
                this._statusBar.className = 'status-bar error';
                this._statusBar.innerHTML = `<span class="status-icon">❌</span><span class="status-text">Build failed (exit ${data.returncode ?? '?'}). See output below.</span>`;
            }
        } catch (err) {
            this._statusBar.className = 'status-bar error';
            this._statusBar.innerHTML = `<span class="status-icon">❌</span><span class="status-text">Request failed: ${_esc(String(err))}</span>`;
        } finally {
            this._buildBtn.disabled = false;
            this._buildBtn.textContent = '▶ Rebuild Docs';
        }
    }

    _showEmpty() {
        this._previewContainer.innerHTML = `
            <div class="preview-empty">
                📄 No documentation yet.<br>
                <small style="color:#9ca3af">Click <strong>Build Docs</strong> to generate with Doxygen.</small>
            </div>`;
        this._frame = null;
    }

    _showPreview() {
        // Use / create iframe
        if (!this._frame) {
            this._frame = document.createElement('iframe');
            this._frame.className = 'preview-frame';
            this._frame.style.height = this._heightSelect.value;
            this._frame.setAttribute('sandbox', 'allow-same-origin allow-scripts allow-forms');
            this._frame.title = 'Documentation Preview';
            this._previewContainer.innerHTML = '';
            this._previewContainer.appendChild(this._frame);
        }
        // Force reload by bumping cache buster
        this._frame.src = `/docs-preview/index.html?t=${Date.now()}`;
    }

    _refreshPreview() {
        this._showPreview();
    }
}

customElements.define('doc-builder', DocBuilder);
