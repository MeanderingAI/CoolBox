/**
 * <demo-viewer>
 * Package Builder sub-tab: left sidebar lists demos from _internal_workspace/demo_workspaces/,
 * right panel shows the selected demo in an iframe.
 * Endpoint: GET  /demos          → { demos: [{ folder, title, description, icon, has_index }] }
 *           POST /demos/new      → { name } → creates a new demo workspace
 *           GET  /demo/{folder}  → serves the demo's index.html
 */

const STYLE = `
:host { display: block; font-family: inherit; height: 100%; }

/* ── Layout ── */
.layout {
    display: flex;
    height: 100%;
    min-height: 500px;
    gap: 0;
}

/* ── Sidebar ── */
.sidebar {
    width: 220px;
    flex-shrink: 0;
    border-right: 1px solid #dde1ea;
    display: flex;
    flex-direction: column;
    background: #f8fafc;
}
.sidebar-header {
    padding: 0.75em 0.9em 0.5em;
    border-bottom: 1px solid #dde1ea;
    display: flex;
    align-items: center;
    justify-content: space-between;
}
.sidebar-title {
    font-size: 0.7em;
    font-weight: 800;
    letter-spacing: 0.1em;
    text-transform: uppercase;
    color: #64748b;
}
.add-btn {
    padding: 0.22em 0.6em;
    font-size: 0.72em;
    font-family: inherit;
    font-weight: 700;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 4px;
    cursor: pointer;
    color: #374151;
    transition: background 0.12s, border-color 0.12s;
}
.add-btn:hover { background: #eff6ff; border-color: #3b82f6; color: #2563eb; }

.demo-list {
    flex: 1;
    overflow-y: auto;
    padding: 0.4em 0;
}

/* ── Demo item ── */
.demo-item {
    display: flex;
    align-items: flex-start;
    gap: 0.5em;
    padding: 0.55em 0.9em;
    cursor: pointer;
    border-left: 3px solid transparent;
    transition: background 0.1s, border-color 0.1s;
    user-select: none;
}
.demo-item:hover { background: #f0f4f8; }
.demo-item.active {
    background: #eff6ff;
    border-left-color: #2563eb;
}
.di-icon { font-size: 1.1em; flex-shrink: 0; margin-top: 1px; }
.di-info { min-width: 0; }
.di-title {
    font-size: 0.8em;
    font-weight: 600;
    color: #1e293b;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}
.demo-item.active .di-title { color: #2563eb; }
.di-desc {
    font-size: 0.68em;
    color: #94a3b8;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.sidebar-empty {
    padding: 1.5em 1em;
    text-align: center;
    font-size: 0.78em;
    color: #94a3b8;
}

/* ── Content area ── */
.content {
    flex: 1;
    display: flex;
    flex-direction: column;
    min-width: 0;
    background: #fff;
}

/* ── Demo header bar ── */
.demo-header {
    display: flex;
    align-items: center;
    gap: 0.6em;
    padding: 0.55em 1em;
    background: #f5f6fa;
    border-bottom: 1px solid #dde1ea;
    flex-shrink: 0;
}
.dh-icon  { font-size: 1.1em; }
.dh-title { font-size: 0.85em; font-weight: 700; color: #1e293b; flex: 1; }
.dh-folder {
    font-size: 0.68em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #94a3b8;
}
.new-tab-btn {
    padding: 0.2em 0.55em;
    font-size: 0.72em;
    font-family: inherit;
    font-weight: 600;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 4px;
    cursor: pointer;
    color: #374151;
    text-decoration: none;
    display: inline-block;
    transition: background 0.12s;
}
.new-tab-btn:hover { background: #f0f4f8; }

/* ── Iframe ── */
.iframe-wrap { flex: 1; overflow: hidden; }
iframe {
    width: 100%;
    height: 100%;
    border: none;
    display: block;
}

/* ── Empty/placeholder state ── */
.placeholder {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    color: #94a3b8;
    gap: 0.5em;
    padding: 2em;
}
.ph-icon  { font-size: 3rem; }
.ph-title { font-size: 0.95em; font-weight: 700; color: #64748b; }
.ph-sub   { font-size: 0.78em; color: #94a3b8; text-align: center; }

/* ── Add demo modal overlay ── */
.modal-overlay {
    position: fixed;
    inset: 0;
    background: rgba(0,0,0,0.45);
    display: flex;
    align-items: center;
    justify-content: center;
    z-index: 9999;
    animation: fadeIn 0.12s ease;
}
@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
.modal {
    background: #fff;
    border-radius: 10px;
    padding: 1.4em 1.5em;
    width: 320px;
    box-shadow: 0 12px 40px rgba(0,0,0,0.25);
    display: flex;
    flex-direction: column;
    gap: 0.85em;
    animation: slideUp 0.14s ease;
}
@keyframes slideUp { from { transform: translateY(8px); opacity: 0; } to { transform: translateY(0); opacity: 1; } }
.modal-title { font-size: 0.95em; font-weight: 800; color: #1e293b; }
.modal input[type=text] {
    width: 100%;
    padding: 0.45em 0.7em;
    font-size: 0.85em;
    font-family: inherit;
    border: 1.5px solid #c5cad8;
    border-radius: 6px;
    outline: none;
    transition: border-color 0.15s;
}
.modal input[type=text]:focus { border-color: #3b82f6; }
.modal-err { font-size: 0.74em; color: #dc2626; min-height: 1em; }
.modal-btns { display: flex; gap: 0.5em; justify-content: flex-end; }
.modal-btns button {
    padding: 0.38em 1em;
    font-size: 0.8em;
    font-family: inherit;
    font-weight: 600;
    border-radius: 5px;
    border: 1px solid #c5cad8;
    cursor: pointer;
    background: #fff;
    color: #374151;
    transition: background 0.12s;
}
.modal-btns .primary-btn {
    background: #2563eb;
    border-color: #2563eb;
    color: #fff;
}
.modal-btns .primary-btn:hover { background: #1d4ed8; }
.modal-btns button:not(.primary-btn):hover { background: #f0f4f8; }

/* ── Loading ── */
.loading-row {
    padding: 1em 0.9em;
    font-size: 0.78em;
    color: #94a3b8;
}

/* ── View tabs (Demo / Code / Libraries) ── */
.view-tabs {
    display: flex;
    background: #f5f6fa;
    border-bottom: 1px solid #dde1ea;
    flex-shrink: 0;
}
.vtab {
    padding: 0.5em 1.1em;
    font-size: 0.75em;
    font-weight: 600;
    cursor: pointer;
    border: none;
    border-bottom: 2px solid transparent;
    background: none;
    color: #64748b;
    font-family: inherit;
    transition: color 0.1s, background 0.1s;
}
.vtab:hover { color: #1e293b; background: #eef2f7; }
.vtab.active { color: #2563eb; border-bottom-color: #2563eb; }

/* ── View panes ── */
.view-pane { display: none; flex: 1; overflow: hidden; }
.view-pane.active { display: flex; flex-direction: column; }

/* ── Code pane ── */
.code-view {
    flex: 1;
    overflow: auto;
    background: #0f1117;
    padding: 1em 1.2em;
}
.code-view pre {
    margin: 0;
    font-family: 'Cascadia Code', 'Consolas', 'Monaco', monospace;
    font-size: 0.74em;
    line-height: 1.65;
    color: #e2e8f0;
    white-space: pre;
    tab-size: 2;
}

/* ── Libraries pane ── */
.lib-view {
    flex: 1;
    overflow-y: auto;
    padding: 0.85em 1em;
    display: flex;
    flex-direction: column;
    gap: 0.75em;
    background: #f8fafc;
}
.lib-card {
    background: #fff;
    border: 1px solid #dde1ea;
    border-radius: 8px;
    overflow: hidden;
}
.lib-card-hdr {
    display: flex;
    align-items: center;
    gap: 0.6em;
    padding: 0.6em 0.85em;
    background: #f0f4f8;
    border-bottom: 1px solid #dde1ea;
}
.lib-card-label {
    font-size: 0.8em;
    font-weight: 700;
    color: #1e293b;
    flex: 1;
}
.lib-card-path {
    font-size: 0.64em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #94a3b8;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    max-width: 320px;
}
.lib-card-desc {
    font-size: 0.72em;
    color: #475569;
    padding: 0.45em 0.85em;
    background: #fafbfc;
    border-bottom: 1px solid #eef2f7;
    line-height: 1.55;
}
.lib-code-wrap {
    max-height: 380px;
    overflow: auto;
    background: #0f1117;
}
.lib-code-wrap pre {
    margin: 0;
    padding: 0.8em 1em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.72em;
    line-height: 1.6;
    color: #e2e8f0;
    white-space: pre;
    tab-size: 2;
}
.pane-msg {
    padding: 2.5em;
    text-align: center;
    font-size: 0.8em;
    color: #94a3b8;
    line-height: 1.7;
}
`;

class DemoViewer extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        const style  = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);
        this._shadow = shadow;

        // ── Layout ──
        const layout = document.createElement('div');
        layout.className = 'layout';

        // Sidebar
        const sidebar = document.createElement('div');
        sidebar.className = 'sidebar';

        const sideHeader = document.createElement('div');
        sideHeader.className = 'sidebar-header';
        sideHeader.innerHTML = '<span class="sidebar-title">Demos</span>';
        const addBtn = document.createElement('button');
        addBtn.className = 'add-btn';
        addBtn.textContent = '＋ Add Demo';
        addBtn.addEventListener('click', () => this._showAddModal());
        sideHeader.appendChild(addBtn);
        sidebar.appendChild(sideHeader);

        this._demoList = document.createElement('div');
        this._demoList.className = 'demo-list';
        sidebar.appendChild(this._demoList);

        // Content area
        const content = document.createElement('div');
        content.className = 'content';
        this._content = content;

        this._showPlaceholder();

        layout.appendChild(sidebar);
        layout.appendChild(content);
        shadow.appendChild(layout);

        this._activeFolder = null;
        this._load();
    }

    async _load() {
        this._demoList.innerHTML = '<div class="loading-row">Loading…</div>';
        let data;
        try {
            const r = await fetch('/demos');
            data = await r.json();
        } catch (e) {
            this._demoList.innerHTML = `<div class="loading-row">Failed to load demos: ${this._esc(e.message)}</div>`;
            return;
        }

        const demos = data.demos || [];
        this._demoList.innerHTML = '';
        if (!demos.length) {
            this._demoList.innerHTML = '<div class="sidebar-empty">No demos yet.<br>Click ＋ Add Demo to create one.</div>';
            return;
        }
        for (const demo of demos) {
            this._demoList.appendChild(this._makeItem(demo));
        }

        // Auto-select first demo
        if (demos.length && !this._activeFolder) {
            this._selectDemo(demos[0]);
        }
    }

    _makeItem(demo) {
        const item = document.createElement('div');
        item.className = 'demo-item' + (this._activeFolder === demo.folder ? ' active' : '');
        item.innerHTML = `
            <span class="di-icon">${this._esc(demo.icon || '🗂')}</span>
            <div class="di-info">
                <div class="di-title">${this._esc(demo.title || demo.folder)}</div>
                ${demo.description ? `<div class="di-desc">${this._esc(demo.description)}</div>` : ''}
            </div>`;
        item.addEventListener('click', () => this._selectDemo(demo));
        item._demo = demo;
        return item;
    }

    _selectDemo(demo) {
        this._activeFolder = demo.folder;

        // Update active state in sidebar
        for (const item of this._demoList.querySelectorAll('.demo-item')) {
            item.classList.toggle('active', item._demo && item._demo.folder === demo.folder);
        }

        this._content.innerHTML = '';

        if (!demo.has_index) {
            this._showPlaceholder('No index.html found for this demo.');
            return;
        }

        // ── Header bar ──
        const header = document.createElement('div');
        header.className = 'demo-header';
        header.innerHTML = `
            <span class="dh-icon">${this._esc(demo.icon || '🗂')}</span>
            <span class="dh-title">${this._esc(demo.title || demo.folder)}</span>
            <span class="dh-folder">_internal_workspace/demo_workspaces/${this._esc(demo.folder)}</span>`;
        const newTabLink = document.createElement('a');
        newTabLink.className = 'new-tab-btn';
        newTabLink.textContent = '↗ New tab';
        newTabLink.href = `/demo/${encodeURIComponent(demo.folder)}`;
        newTabLink.target = '_blank';
        newTabLink.rel = 'noopener noreferrer';
        header.appendChild(newTabLink);
        this._content.appendChild(header);

        // ── Iframe ──
        const wrap = document.createElement('div');
        wrap.className = 'iframe-wrap';
        const iframe = document.createElement('iframe');
        iframe.src = `/demo/${encodeURIComponent(demo.folder)}`;
        iframe.title = demo.title || demo.folder;
        iframe.setAttribute('sandbox', 'allow-same-origin allow-scripts allow-forms');
        wrap.appendChild(iframe);
        this._content.appendChild(wrap);
    }

    async _loadCode(folder, pane) {
        pane.innerHTML = '<div class="pane-msg">Loading source…</div>';
        try {
            const r = await fetch(`/demo/${encodeURIComponent(folder)}/source`);
            if (!r.ok) throw new Error(`HTTP ${r.status}`);
            const src = await r.text();
            const pre = document.createElement('pre');
            pre.textContent = src;
            const view = document.createElement('div');
            view.className = 'code-view';
            view.appendChild(pre);
            pane.innerHTML = '';
            pane.appendChild(view);
        } catch (e) {
            pane.innerHTML = `<div class="pane-msg">Could not load source: ${this._esc(e.message)}</div>`;
        }
    }

    async _loadLibraries(libraries, pane) {
        if (!libraries || !libraries.length) {
            pane.innerHTML = '<div class="lib-view"><div class="pane-msg">No libraries listed for this demo.<br>Add a <code>libraries</code> array to <code>demo.json</code>.</div></div>';
            return;
        }
        const view = document.createElement('div');
        view.className = 'lib-view';
        pane.appendChild(view);

        for (const lib of libraries) {
            const card = document.createElement('div');
            card.className = 'lib-card';
            card.innerHTML = `
                <div class="lib-card-hdr">
                    <span class="lib-card-label">📄 ${this._esc(lib.label || lib.path)}</span>
                    <span class="lib-card-path">${this._esc(lib.path || '')}</span>
                </div>
                ${lib.description ? `<div class="lib-card-desc">${this._esc(lib.description)}</div>` : ''}
                <div class="lib-code-wrap"><div class="pane-msg" style="background:#0f1117;color:#475569">Loading…</div></div>`;
            view.appendChild(card);

            const codeWrap = card.querySelector('.lib-code-wrap');
            fetch('/file-content?' + new URLSearchParams({ path: lib.path }))
                .then(r => r.ok ? r.text() : r.json().then(d => Promise.reject(d.error || r.statusText)))
                .then(src => {
                    const pre = document.createElement('pre');
                    pre.textContent = src;
                    codeWrap.innerHTML = '';
                    codeWrap.appendChild(pre);
                })
                .catch(err => {
                    codeWrap.innerHTML = `<div class="pane-msg" style="background:#0f1117;color:#ef4444">Error: ${this._esc(String(err))}</div>`;
                });
        }
    }

    _showPlaceholder(msg) {
        this._content.innerHTML = '';
        const ph = document.createElement('div');
        ph.className = 'placeholder';
        ph.innerHTML = `
            <div class="ph-icon">🎬</div>
            <div class="ph-title">Select a demo</div>
            <div class="ph-sub">${this._esc(msg || 'Choose a demo from the sidebar or add a new one.')}</div>`;
        this._content.appendChild(ph);
    }

    _showAddModal() {
        const overlay = document.createElement('div');
        overlay.className = 'modal-overlay';
        const modal = document.createElement('div');
        modal.className = 'modal';
        modal.innerHTML = `
            <div class="modal-title">＋ New Demo</div>
            <input type="text" id="new-demo-name" placeholder="e.g. Neural Network Viz" maxlength="60" />
            <div class="modal-err" id="new-demo-err"></div>
            <div class="modal-btns">
                <button id="new-demo-cancel">Cancel</button>
                <button class="primary-btn" id="new-demo-ok">Create</button>
            </div>`;
        overlay.appendChild(modal);
        this._shadow.appendChild(overlay);

        const nameInput = modal.querySelector('#new-demo-name');
        const errEl     = modal.querySelector('#new-demo-err');
        const okBtn     = modal.querySelector('#new-demo-ok');
        const cancelBtn = modal.querySelector('#new-demo-cancel');

        nameInput.focus();

        const close = () => overlay.remove();

        cancelBtn.addEventListener('click', close);
        overlay.addEventListener('click', e => { if (e.target === overlay) close(); });

        const submit = async () => {
            const name = nameInput.value.trim();
            if (!name) { errEl.textContent = 'Please enter a name.'; return; }
            okBtn.disabled = true;
            okBtn.textContent = 'Creating…';
            try {
                const r = await fetch('/demos/new', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name }),
                });
                const data = await r.json();
                if (!r.ok) { errEl.textContent = data.error || 'Failed to create demo.'; okBtn.disabled = false; okBtn.textContent = 'Create'; return; }
                close();
                await this._load();
                // Select the new demo
                const newItem = [...this._demoList.querySelectorAll('.demo-item')]
                    .find(el => el._demo && el._demo.folder === data.folder);
                if (newItem) newItem.click();
            } catch (e) {
                errEl.textContent = 'Error: ' + e.message;
                okBtn.disabled = false;
                okBtn.textContent = 'Create';
            }
        };

        okBtn.addEventListener('click', submit);
        nameInput.addEventListener('keydown', e => { if (e.key === 'Enter') submit(); if (e.key === 'Escape') close(); });
    }

    _esc(s) {
        return String(s ?? '')
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }
}

customElements.define('demo-viewer', DemoViewer);
