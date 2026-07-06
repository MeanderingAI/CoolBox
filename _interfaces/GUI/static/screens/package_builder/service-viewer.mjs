// Custom element <service-viewer>
// Displays all known local services grouped by category (Distribution Storage,
// Distribution Tag, Language Servers / LSP) with launch, stop and build controls.
// Data source: GET /network/local-services  (FastAPI, main.py)
// Used in: Package Builder → 🔧 Services tab.

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Toolbar ─────────────────────────────────────────── */
.toolbar {
    display: flex;
    align-items: center;
    gap: 0.5em;
    margin-bottom: 1em;
}
.btn {
    padding: 0.3em 0.8em;
    font-size: 0.8em;
    font-family: inherit;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    background: #fff;
    cursor: pointer;
    color: #374151;
    transition: background 0.12s;
}
.btn:hover:not(:disabled) { background: #e8eaf0; }
.btn:disabled { opacity: 0.45; cursor: default; }
.btn-launch { border-color: #86efac; color: #15803d; }
.btn-launch:hover:not(:disabled) { background: #f0fdf4; }
.btn-danger  { border-color: #fca5a5; color: #b91c1c; }
.btn-danger:hover:not(:disabled) { background: #fef2f2; }
.btn-build  { border-color: #a5b4fc; color: #4338ca; }
.btn-build:hover:not(:disabled) { background: #eef2ff; }
.status-txt { font-size: 0.78em; color: #6b7280; margin-left: auto; }

/* ── Group section ───────────────────────────────────── */
.group-section { margin-bottom: 1.6em; }
.group-hdr {
    display: flex;
    align-items: center;
    gap: 0.5em;
    font-size: 0.72em;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.09em;
    color: #6b7280;
    padding-bottom: 0.4em;
    margin-bottom: 0.65em;
    border-bottom: 1px solid #e5e7eb;
}
.group-count {
    margin-left: auto;
    font-weight: normal;
    text-transform: none;
    letter-spacing: 0;
    color: #9ca3af;
}

/* ── Service card grid ───────────────────────────────── */
.svc-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(290px, 1fr));
    gap: 0.85em;
}
.svc-card {
    background: #f8faff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1em 1.1em;
    display: flex;
    flex-direction: column;
    gap: 0;
}
.svc-card.running   { border-color: #86efac; background: #f0fdf4; }
.svc-card.not-built { border-color: #fde68a; background: #fffbeb; }
.svc-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
    margin-bottom: 0.45em;
}
.svc-dot {
    width: 9px; height: 9px;
    border-radius: 50%;
    flex-shrink: 0;
}
.dot-green { background: #16a34a; }
.dot-gray  { background: #9ca3af; }
.dot-amber { background: #d97706; }
.svc-label {
    font-size: 0.89em;
    font-weight: 700;
    color: #1f2937;
    flex: 1;
}
.svc-badge {
    font-size: 0.66em;
    padding: 0.15em 0.5em;
    border-radius: 999px;
    font-weight: 600;
    white-space: nowrap;
}
.badge-run     { background: #dcfce7; color: #15803d; }
.badge-stop    { background: #f3f4f6; color: #6b7280; }
.badge-nobuild { background: #fef3c7; color: #92400e; }
.svc-desc {
    font-size: 0.77em;
    color: #6b7280;
    margin-bottom: 0.7em;
    line-height: 1.5;
    flex: 1;
}
.svc-meta {
    font-size: 0.71em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #9ca3af;
    margin-bottom: 0.6em;
}
.svc-actions { display: flex; gap: 0.4em; flex-wrap: wrap; }

/* ── Empty / error states ────────────────────────────── */
.empty {
    text-align: center;
    padding: 2.5em 1em;
    color: #9ca3af;
    font-size: 0.87em;
}
.empty-icon { font-size: 2rem; margin-bottom: 0.5em; }
.error-txt  { font-size: 0.83em; color: #dc2626; padding: 0.5em 0; }
`;

// ── Helpers ───────────────────────────────────────────────────────────────────

function makeBtn(text, cls = '') {
    const b = document.createElement('button');
    b.className = `btn${cls ? ' ' + cls : ''}`;
    b.textContent = text;
    return b;
}

// ── Component ─────────────────────────────────────────────────────────────────

class ServiceViewer extends HTMLElement {
    connectedCallback() {
        if (this._built) return;
        this._built = true;

        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';

        const refreshBtn = makeBtn('↻ Refresh');
        const statusTxt  = document.createElement('span');
        statusTxt.className = 'status-txt';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(statusTxt);
        shadow.appendChild(toolbar);

        // Content area (groups of service cards)
        const content = document.createElement('div');
        shadow.appendChild(content);

        this._content   = content;
        this._statusTxt = statusTxt;
        this._shadow    = shadow;

        refreshBtn.addEventListener('click', () => this._refresh());
        this._refresh();
    }

    async _refresh() {
        this._statusTxt.textContent = 'Loading…';
        try {
            const data = await fetch('/network/local-services').then(r => r.json());
            this._render(data.services || []);
            this._statusTxt.textContent = `Updated ${new Date().toLocaleTimeString()}`;
        } catch (err) {
            this._statusTxt.textContent = `Error: ${err.message}`;
            const e = document.createElement('div');
            e.className = 'error-txt';
            e.textContent = `Failed to load services: ${err.message}`;
            this._content.innerHTML = '';
            this._content.appendChild(e);
        }
    }

    _render(services) {
        this._content.innerHTML = '';

        if (!services.length) {
            const empty = document.createElement('div');
            empty.className = 'empty';
            empty.innerHTML = '<div class="empty-icon">🔧</div><div>No services defined.</div>';
            this._content.appendChild(empty);
            return;
        }

        // Group by the "group" field
        const groups = {};
        const groupOrder = [];
        for (const svc of services) {
            const g = svc.group || 'Other';
            if (!groups[g]) { groups[g] = []; groupOrder.push(g); }
            groups[g].push(svc);
        }

        const GROUP_ICONS = {
            'Distribution Storage': '🗄️',
            'Distribution Tag':     '🏷️',
            'Language Servers (LSP)': '💬',
        };

        for (const g of groupOrder) {
            const section = document.createElement('div');
            section.className = 'group-section';

            const hdr = document.createElement('div');
            hdr.className = 'group-hdr';
            const icon = GROUP_ICONS[g] || '⚙️';
            hdr.innerHTML = `${icon} ${g}<span class="group-count">${groups[g].length} service${groups[g].length !== 1 ? 's' : ''}</span>`;
            section.appendChild(hdr);

            const grid = document.createElement('div');
            grid.className = 'svc-grid';
            for (const svc of groups[g]) {
                grid.appendChild(this._makeCard(svc));
            }
            section.appendChild(grid);
            this._content.appendChild(section);
        }
    }

    _makeCard(svc) {
        const card = document.createElement('div');
        const dotCls   = svc.running ? 'dot-green' : (svc.built ? 'dot-gray' : 'dot-amber');
        const badgeTxt = svc.running ? 'Running'  : (svc.built ? 'Stopped' : 'Not Built');
        const badgeCls = svc.running ? 'badge-run': (svc.built ? 'badge-stop' : 'badge-nobuild');
        card.className = `svc-card${svc.running ? ' running' : (!svc.built ? ' not-built' : '')}`;

        const hdr = document.createElement('div');
        hdr.className = 'svc-header';
        hdr.innerHTML = `
            <div class="svc-dot ${dotCls}"></div>
            <div class="svc-label">${svc.label}</div>
            <span class="svc-badge ${badgeCls}">${badgeTxt}</span>`;
        card.appendChild(hdr);

        const desc = document.createElement('div');
        desc.className = 'svc-desc';
        desc.textContent = svc.description;
        card.appendChild(desc);

        const meta = document.createElement('div');
        meta.className = 'svc-meta';
        meta.textContent = `exe: ${svc.exe}${svc.pid ? `  ·  pid: ${svc.pid}` : ''}`;
        card.appendChild(meta);

        // Action buttons
        const actions = document.createElement('div');
        actions.className = 'svc-actions';

        const launchBtn = makeBtn('▶ Launch', 'btn-launch');
        launchBtn.disabled = svc.running || !svc.built;

        const stopBtn = makeBtn('■ Stop', 'btn-danger');
        stopBtn.disabled = !svc.running;

        const buildBtn = makeBtn('🔨 Build', 'btn-build');
        buildBtn.title = 'Open Package Builder → Apps to build this executable';
        buildBtn.addEventListener('click', () => {
            document.dispatchEvent(new CustomEvent('coolbox:navigate', {
                detail: { tab: 'libraries', subtab: 'apps' }
            }));
        });

        launchBtn.addEventListener('click', async () => {
            launchBtn.disabled = true;
            launchBtn.textContent = 'Launching…';
            try {
                const res = await fetch('/network/local-services/launch', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id: svc.id }),
                }).then(r => r.json());
                if (!res.success) alert(`Launch failed: ${res.error || 'Unknown error'}`);
            } catch (e) {
                alert(`Launch error: ${e.message}`);
            }
            setTimeout(() => this._refresh(), 600);
        });

        stopBtn.addEventListener('click', async () => {
            stopBtn.disabled = true;
            stopBtn.textContent = 'Stopping…';
            try {
                const res = await fetch('/network/local-services/stop', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ id: svc.id }),
                }).then(r => r.json());
                if (!res.success) alert(`Stop failed: ${res.error || 'Unknown error'}`);
            } catch (e) {
                alert(`Stop error: ${e.message}`);
            }
            setTimeout(() => this._refresh(), 600);
        });

        actions.appendChild(launchBtn);
        actions.appendChild(stopBtn);
        actions.appendChild(buildBtn);
        card.appendChild(actions);
        return card;
    }
}

customElements.define('service-viewer', ServiceViewer);
