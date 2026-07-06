/**
 * <editor-screen group="COMMS" lib="LSP">
 * A two-pane header-file browser: collapsible file-sidebar on the left,
 * syntax-highlighted code-shower on the right.
 *
 * Set `group` + `lib` attributes (matching a /library/info response).
 * The element fetches the library structure automatically.
 */
import './sub/file-sidebar.mjs';
import './sub/code-shower.mjs';

const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    border: 1px solid #3e3e42;
    border-radius: 6px;
    overflow: hidden;
    background: #1e1e1e;
    font-family: 'Segoe UI', system-ui, sans-serif;
    min-height: 0;
}

/* ── Toolbar ─────────────────────────────────────────── */
.es-bar {
    display: flex;
    align-items: center;
    gap: 0.6em;
    padding: 0.4em 0.8em;
    background: #2d2d30;
    border-bottom: 1px solid #3e3e42;
    flex-shrink: 0;
}
.es-title {
    font-size: 0.8em;
    color: #ccc;
    font-weight: 600;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.es-meta {
    font-size: 0.72em;
    color: #888;
    white-space: nowrap;
}

/* ── Content area ────────────────────────────────────── */
.es-content {
    display: flex;
    flex: 1;
    min-height: 0;
}

/* ── Sidebar ─────────────────────────────────────────── */
.es-sidebar {
    width: 220px;
    min-width: 140px;
    max-width: 320px;
    flex-shrink: 0;
    display: flex;
    overflow: hidden;
}
file-sidebar {
    flex: 1;
    min-width: 0;
}

/* ── Drag handle ─────────────────────────────────────── */
.es-drag {
    width: 4px;
    cursor: col-resize;
    background: #3e3e42;
    flex-shrink: 0;
    transition: background 0.12s;
}
.es-drag:hover, .es-drag.dragging { background: #0e639c; }

/* ── Viewer ──────────────────────────────────────────── */
.es-viewer {
    flex: 1;
    min-width: 0;
    display: flex;
    overflow: hidden;
}
code-shower {
    flex: 1;
    min-width: 0;
}

/* ── States ──────────────────────────────────────────── */
.es-state {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100%;
    color: #666;
    font-size: 0.85em;
}
.es-state.error { color: #f48771; }
`;

class EditorScreen extends HTMLElement {
    static get observedAttributes() { return ['group', 'lib']; }

    connectedCallback() {
        if (!this.shadowRoot) this._build();
        this._maybeLoad();
    }

    attributeChangedCallback(name, old, newVal) {
        if ((name === 'group' || name === 'lib') && this.shadowRoot && newVal !== old) {
            this._maybeLoad();
        }
    }

    _build() {
        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // ── Toolbar ──
        const bar = document.createElement('div');
        bar.className = 'es-bar';

        this._title = document.createElement('span');
        this._title.className = 'es-title';
        this._title.textContent = 'Library Files';
        bar.appendChild(this._title);

        this._meta = document.createElement('span');
        this._meta.className = 'es-meta';
        bar.appendChild(this._meta);

        shadow.appendChild(bar);

        // ── Content ──
        const content = document.createElement('div');
        content.className = 'es-content';

        // Left sidebar wrapper
        const sidebarWrap = document.createElement('div');
        sidebarWrap.className = 'es-sidebar';
        this._sidebarWrap = sidebarWrap;

        this._sidebar = document.createElement('file-sidebar');
        sidebarWrap.appendChild(this._sidebar);
        content.appendChild(sidebarWrap);

        // Drag handle
        const drag = document.createElement('div');
        drag.className = 'es-drag';
        content.appendChild(drag);
        this._initDrag(drag, sidebarWrap);

        // Right viewer
        const viewerWrap = document.createElement('div');
        viewerWrap.className = 'es-viewer';

        this._viewer = document.createElement('code-shower');
        viewerWrap.appendChild(this._viewer);
        content.appendChild(viewerWrap);

        shadow.appendChild(content);

        // Route file-select events from sidebar to viewer
        this._sidebar.addEventListener('file-select', (e) => {
            this._viewer.setAttribute('src', e.detail.path);
            this._title.textContent = `${this.getAttribute('group') || ''} / ${this.getAttribute('lib') || ''} — ${e.detail.name}`;
        });

        this._loaded = '';
    }

    _maybeLoad() {
        const group = this.getAttribute('group') || '';
        const lib   = this.getAttribute('lib') || '';
        const key   = `${group}/${lib}`;
        if (!group || !lib || key === this._loaded) return;
        this._loaded = key;
        this._title.textContent = `${group} / ${lib}`;
        this._meta.textContent = 'Loading…';
        this._load(group, lib);
    }

    async _load(group, lib) {
        try {
            const r = await fetch(`/library/info?group=${encodeURIComponent(group)}&lib=${encodeURIComponent(lib)}`);
            const data = await r.json();
            if (data.error) throw new Error(data.error);

            const libs = data.libs || [];
            const headerCount = libs.reduce((n, l) => n + (l.headers?.length || 0), 0);
            this._meta.textContent = `${libs.length} lib${libs.length !== 1 ? 's' : ''} · ${headerCount} header${headerCount !== 1 ? 's' : ''}`;
            this._title.textContent = `${group} / ${lib}`;

            this._sidebar.setLibs(libs);

            // Auto-select first header if available
            const firstHdr = libs[0]?.headers?.[0];
            if (firstHdr) {
                this._viewer.setAttribute('src', firstHdr.path);
                this._title.textContent = `${group} / ${lib} — ${firstHdr.name}`;
            }
        } catch (e) {
            this._meta.textContent = '';
            this._title.textContent = `Error: ${e.message}`;
        }
    }

    // ── Drag-to-resize sidebar ──────────────────────────────
    _initDrag(handle, sidebarWrap) {
        let startX = 0;
        let startW = 0;
        let dragging = false;

        const onMove = (e) => {
            if (!dragging) return;
            const dx = (e.clientX || e.touches?.[0]?.clientX || startX) - startX;
            const newW = Math.max(120, Math.min(420, startW + dx));
            sidebarWrap.style.width = `${newW}px`;
        };

        const onUp = () => {
            dragging = false;
            handle.classList.remove('dragging');
            document.removeEventListener('mousemove', onMove);
            document.removeEventListener('mouseup', onUp);
        };

        handle.addEventListener('mousedown', (e) => {
            e.preventDefault();
            dragging = true;
            startX = e.clientX;
            startW = sidebarWrap.offsetWidth;
            handle.classList.add('dragging');
            document.addEventListener('mousemove', onMove);
            document.addEventListener('mouseup', onUp);
        });
    }
}

customElements.define('editor-screen', EditorScreen);
