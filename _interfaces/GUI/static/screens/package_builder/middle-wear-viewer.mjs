/**
 * <middle-wear-viewer>
 * Package Builder sub-tab: shows middleware tools from business_suite/middle_wear/.
 * Clicking "Open Tool" loads the tool in an iframe below the card grid.
 * Endpoint: GET /middle-wear  →  { tools: [{ folder, has_index }] }
 *           GET /middle-portal/{folder}  →  serves business_suite/middle_wear/{folder}/index.html
 */

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Toolbar ── */
.toolbar {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.5em 0 0.85em;
}
.btn {
    padding: 0.3em 0.75em;
    font-size: 0.8em;
    font-family: inherit;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    background: #fff;
    cursor: pointer;
    color: #374151;
    transition: background 0.12s;
}
.btn:hover { background: #f0f4f8; }
.status { font-size: 0.78em; color: #9ca3af; margin-left: auto; }

/* ── Tool grid ── */
.tool-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 1em;
    margin-bottom: 1em;
}

/* ── Tool card ── */
.tool-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1.1em 1.2em;
    display: flex;
    flex-direction: column;
    gap: 0.5em;
    transition: border-color 0.15s, box-shadow 0.15s;
}
.tool-card:hover {
    border-color: #7c3aed;
    box-shadow: 0 3px 14px rgba(124,58,237,0.09);
}
.tool-card.active {
    border-color: #7c3aed;
    background: #faf5ff;
}

/* ── Card header ── */
.tc-header { display: flex; align-items: center; gap: 0.6em; }
.tc-icon   { font-size: 1.5em; }
.tc-title  { font-size: 0.95em; font-weight: 700; color: #1e293b; flex: 1; }
.tc-folder {
    font-size: 0.7em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #94a3b8;
}
.tc-desc { font-size: 0.8em; color: #64748b; line-height: 1.5; }

/* ── Card actions ── */
.tc-actions { display: flex; gap: 0.4em; flex-wrap: wrap; margin-top: 0.25em; }

.ac-btn {
    padding: 0.25em 0.7em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 4px;
    cursor: pointer;
    font-weight: 600;
    border: 1px solid transparent;
    text-decoration: none;
    transition: background 0.12s, opacity 0.12s;
    display: inline-block;
}
.ac-btn.open {
    background: #7c3aed;
    color: #fff;
    border-color: #7c3aed;
}
.ac-btn.open:hover { background: #6d28d9; }
.ac-btn.open.active { background: #5b21b6; }
.ac-btn.ext {
    background: #f8fafc;
    border-color: #cbd5e1;
    color: #475569;
}
.ac-btn.ext:hover { background: #f1f5f9; }

/* ── Iframe viewer ── */
.viewer-wrap {
    border: 1.5px solid #ddd6fe;
    border-radius: 10px;
    overflow: hidden;
    animation: slideIn 0.18s ease;
}
@keyframes slideIn {
    from { opacity: 0; transform: translateY(-6px); }
    to   { opacity: 1; transform: translateY(0); }
}
.viewer-header {
    background: #7c3aed;
    color: #fff;
    padding: 0.5em 1em;
    display: flex;
    align-items: center;
    gap: 0.5em;
    font-size: 0.82em;
    font-weight: 600;
}
.viewer-header .vh-title { flex: 1; }
.viewer-close {
    padding: 0.15em 0.5em;
    border-radius: 4px;
    background: rgba(255,255,255,0.15);
    border: 1px solid rgba(255,255,255,0.25);
    color: #fff;
    cursor: pointer;
    font-size: 0.85em;
    transition: background 0.12s;
}
.viewer-close:hover { background: rgba(255,255,255,0.28); }
iframe {
    width: 100%;
    height: 600px;
    border: none;
    display: block;
}

/* ── Empty / loading ── */
.empty {
    text-align: center;
    padding: 3em 1em;
    color: #94a3b8;
}
.empty-icon { font-size: 2.5rem; margin-bottom: 0.5em; }
.loading { color: #94a3b8; font-size: 0.88em; padding: 1.5em 0; }
`;

const FOLDER_ICONS = {
    database_management: '🗄️',
    nginx_setup:         '🌐',
    distributed_setup:   '⚙️',
};

const FOLDER_DESCRIPTIONS = {
    database_management: 'Database connection manager — configure PostgreSQL, SQLite, Redis and run queries.',
    nginx_setup:         'Nginx virtual host configurator — server blocks, upstreams, SSL/TLS, headers and logs.',
    distributed_setup:   'Distributed systems host setup — launch and monitor distribution_tag master & workers.',
};

function iconFor(folder) {
    return FOLDER_ICONS[folder] ?? '🔧';
}

function titleFor(folder) {
    return folder.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
}

class MiddleWearViewer extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        this._shadow = shadow;
        this._activeFolder = null;
        this._cards = new Map(); // folder → card element

        // ── Toolbar ──
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'btn';
        refreshBtn.textContent = '↻ Refresh';
        this._statusEl = document.createElement('span');
        this._statusEl.className = 'status';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(this._statusEl);
        shadow.appendChild(toolbar);

        // ── Grid ──
        this._grid = document.createElement('div');
        this._grid.className = 'tool-grid';
        shadow.appendChild(this._grid);

        // ── Viewer (iframe) ──
        this._viewerWrap = null;

        refreshBtn.addEventListener('click', () => this._load());
        this._load();
    }

    async _load() {
        this._statusEl.textContent = 'Loading…';
        this._grid.innerHTML = '<div class="loading">⟳ Fetching middleware tools…</div>';
        try {
            const res = await fetch('/middle-wear');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            this._render(data.tools ?? []);
        } catch (err) {
            this._grid.innerHTML = `<div class="empty"><div class="empty-icon">⚠️</div>Failed to load: ${err.message}</div>`;
            this._statusEl.textContent = '';
        }
    }

    _render(tools) {
        this._grid.innerHTML = '';
        this._cards.clear();

        if (!tools.length) {
            this._grid.innerHTML = '<div class="empty"><div class="empty-icon">🔧</div>No middleware tools found in business_suite/middle_wear/.</div>';
            this._statusEl.textContent = '0 tools';
            return;
        }

        this._statusEl.textContent = `${tools.length} tool${tools.length !== 1 ? 's' : ''}`;

        for (const tool of tools) {
            const card = this._makeCard(tool);
            this._grid.appendChild(card);
            this._cards.set(tool.folder, card);
        }
    }

    _makeCard(tool) {
        const { folder, has_index } = tool;
        const icon  = iconFor(folder);
        const title = titleFor(folder);
        const desc  = FOLDER_DESCRIPTIONS[folder] ?? 'Middleware tool.';

        const card = document.createElement('div');
        card.className = 'tool-card';

        // Header
        const header = document.createElement('div');
        header.className = 'tc-header';
        const iconEl = document.createElement('span');
        iconEl.className = 'tc-icon';
        iconEl.textContent = icon;
        const titleEl = document.createElement('span');
        titleEl.className = 'tc-title';
        titleEl.textContent = title;
        const folderEl = document.createElement('span');
        folderEl.className = 'tc-folder';
        folderEl.textContent = folder;
        header.append(iconEl, titleEl, folderEl);

        // Description
        const descEl = document.createElement('div');
        descEl.className = 'tc-desc';
        descEl.textContent = desc;

        // Actions
        const actions = document.createElement('div');
        actions.className = 'tc-actions';

        const openBtn = document.createElement('button');
        openBtn.className = 'ac-btn open';
        openBtn.textContent = has_index ? `${icon} Open Tool` : '⚠ No index.html';
        openBtn.disabled = !has_index;
        if (!has_index) openBtn.style.opacity = '0.45';

        const extBtn = document.createElement('a');
        extBtn.className = 'ac-btn ext';
        extBtn.textContent = '↗ New tab';
        extBtn.href = `/middle-portal/${folder}`;
        extBtn.target = '_blank';
        extBtn.rel = 'noopener noreferrer';
        if (!has_index) { extBtn.style.opacity = '0.4'; extBtn.style.pointerEvents = 'none'; }

        actions.append(openBtn, extBtn);
        card.append(header, descEl, actions);

        openBtn.addEventListener('click', () => this._openTool(folder, title, icon, card, openBtn));

        return card;
    }

    _openTool(folder, title, icon, card, openBtn) {
        // Deactivate previous
        this._cards.forEach((c, f) => {
            c.classList.remove('active');
            const btn = c.querySelector('.ac-btn.open');
            if (btn) btn.classList.remove('active');
        });

        if (this._activeFolder === folder && this._viewerWrap) {
            // Toggle off
            this._viewerWrap.remove();
            this._viewerWrap = null;
            this._activeFolder = null;
            return;
        }

        this._activeFolder = folder;
        card.classList.add('active');
        openBtn.classList.add('active');

        // Build viewer
        if (this._viewerWrap) this._viewerWrap.remove();
        const wrap = document.createElement('div');
        wrap.className = 'viewer-wrap';

        const header = document.createElement('div');
        header.className = 'viewer-header';
        const vhTitle = document.createElement('span');
        vhTitle.className = 'vh-title';
        vhTitle.textContent = `${icon} ${title}`;
        const closeBtn = document.createElement('button');
        closeBtn.className = 'viewer-close';
        closeBtn.textContent = '✕ Close';
        header.append(vhTitle, closeBtn);

        const iframe = document.createElement('iframe');
        iframe.src = `/middle-portal/${folder}`;
        iframe.title = title;
        iframe.loading = 'lazy';

        wrap.append(header, iframe);
        this._shadow.appendChild(wrap);
        this._viewerWrap = wrap;
        wrap.scrollIntoView({ behavior: 'smooth', block: 'nearest' });

        closeBtn.addEventListener('click', () => {
            wrap.remove();
            this._viewerWrap = null;
            this._activeFolder = null;
            card.classList.remove('active');
            openBtn.classList.remove('active');
        });
    }
}

customElements.define('middle-wear-viewer', MiddleWearViewer);
