// Custom element <network-info>
// Two sub-tabs:
//   📊 Overview        — server / dashboard info tiles
//   🌐 Network Services — ServiceRegistry-style table of what's registered at runtime
// NB: Local service management (distribution_tag, distribution_storage, LSP) has
//     moved to Package Builder → 🔧 Services.

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Sub-tab bar ── */
.sub-bar {
    display: flex;
    gap: 0.4em;
    padding: 0.75em 1em 0;
    background: #f5f6fa;
    border-bottom: 1px solid #dde1ea;
}
.sub-btn {
    padding: 0.35em 1em;
    font-size: 0.82em;
    font-family: inherit;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 5px 5px 0 0;
    border-bottom: none;
    cursor: pointer;
    color: #555;
    margin-bottom: -1px;
    transition: background 0.12s, color 0.12s;
}
.sub-btn:hover { background: #e8eaf0; color: #222; }
.sub-btn[aria-selected="true"] {
    background: #fff;
    border-color: #0e639c;
    color: #0e639c;
    font-weight: 600;
    border-bottom: 1px solid #fff;
}

/* ── Panels ── */
.sub-panel { display: none; padding: 1em; }
.sub-panel.active { display: block; }

/* ── Overview tiles ── */
.ni-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(210px, 1fr));
    gap: 0.75em;
}
.ni-tile {
    background: #f8faff;
    border: 1px solid #dde1ea;
    border-radius: 8px;
    padding: 0.7em 1em;
}
.ni-label {
    font-size: 0.68em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #9ca3af;
    margin-bottom: 0.3em;
}
.ni-value {
    font-size: 0.88em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #1f2937;
    font-weight: 600;
}
.ni-value a { color: #0e639c; text-decoration: none; }
.ni-value a:hover { text-decoration: underline; }
.ok   { color: #16a34a; }
.warn { color: #d97706; }
.ni-note {
    margin-top: 0.75em;
    font-size: 0.8em;
    color: #6b7280;
    font-style: italic;
}

/* ── Toolbar (refresh row) ── */
.toolbar {
    display: flex;
    align-items: center;
    gap: 0.5em;
    margin-bottom: 0.85em;
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
.btn:hover:not(:disabled) { background: #e8eaf0; }
.btn:disabled { opacity: 0.45; cursor: default; }
.btn-danger  { border-color: #fca5a5; color: #b91c1c; }
.btn-danger:hover:not(:disabled) { background: #fef2f2; }
.btn-launch { border-color: #86efac; color: #15803d; }
.btn-launch:hover:not(:disabled) { background: #f0fdf4; }
.btn-build  { border-color: #a5b4fc; color: #4338ca; }
.btn-build:hover { background: #eef2ff; }
.status-txt { font-size: 0.78em; color: #6b7280; margin-left: auto; }

/* ── Service cards ── */
.svc-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
    gap: 0.85em;
}
.svc-card {
    background: #f8faff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1em 1.1em;
}
.svc-card.running { border-color: #86efac; background: #f0fdf4; }
.svc-card.not-built { border-color: #fde68a; background: #fffbeb; }
.svc-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
    margin-bottom: 0.5em;
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
    font-size: 0.9em;
    font-weight: 700;
    color: #1f2937;
    flex: 1;
}
.svc-badge {
    font-size: 0.68em;
    padding: 0.15em 0.5em;
    border-radius: 999px;
    font-weight: 600;
}
.badge-run  { background: #dcfce7; color: #15803d; }
.badge-stop { background: #f3f4f6; color: #6b7280; }
.badge-nobuild { background: #fef3c7; color: #92400e; }
.svc-desc {
    font-size: 0.78em;
    color: #6b7280;
    margin-bottom: 0.75em;
    line-height: 1.5;
}
.svc-meta {
    font-size: 0.73em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #6b7280;
    margin-bottom: 0.6em;
}
.svc-actions { display: flex; gap: 0.4em; }

/* ── Network services table ── */
.ns-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.82em;
}
.ns-table th {
    text-align: left;
    font-size: 0.7em;
    letter-spacing: 0.06em;
    text-transform: uppercase;
    color: #9ca3af;
    font-weight: 700;
    padding: 0.4em 0.6em;
    border-bottom: 2px solid #dde1ea;
}
.ns-table td {
    padding: 0.5em 0.6em;
    border-bottom: 1px solid #f3f4f6;
    color: #374151;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.88em;
}
.ns-table tr:last-child td { border-bottom: none; }
.health-ok   { color: #16a34a; font-weight: 700; }
.health-bad  { color: #dc2626; font-weight: 700; }

/* ── Empty state ── */
.empty {
    text-align: center;
    padding: 2.5em 1em;
    color: #9ca3af;
    font-size: 0.875em;
}
.empty-icon { font-size: 2rem; margin-bottom: 0.5em; }
.empty-sub  { font-size: 0.85em; color: #d1d5db; margin-top: 0.25em; }

/* ── Loading ── */
.loading { color: #9ca3af; font-size: 0.85em; padding: 1em 0; }

/* ── Section dividers ── */
.section-hdr {
    display: flex;
    align-items: center;
    gap: 0.5em;
    font-size: 0.73em;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: #6b7280;
    margin: 0.9em 0 0.5em;
    padding-bottom: 0.35em;
    border-bottom: 1px solid #e5e7eb;
}
.section-hdr-count {
    margin-left: auto;
    font-weight: normal;
    text-transform: none;
    letter-spacing: 0;
    color: #9ca3af;
    font-size: 0.95em;
}

/* ── Ports table ── */
.ports-wrap {
    overflow-x: auto;
    margin-bottom: 1em;
    border: 1px solid #dde1ea;
    border-radius: 8px;
}
.ports-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.77em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    background: #fff;
}
.ports-table th {
    text-align: left;
    font-size: 0.68em;
    text-transform: uppercase;
    letter-spacing: 0.06em;
    color: #9ca3af;
    font-weight: 700;
    padding: 0.45em 0.75em;
    background: #f8fafc;
    border-bottom: 1px solid #dde1ea;
}
.ports-table td {
    padding: 0.38em 0.75em;
    border-bottom: 1px solid #f3f4f6;
    color: #374151;
    vertical-align: middle;
}
.ports-table tr:last-child td { border-bottom: none; }
.ports-table tr.port-known { background: #fafbff; }
.port-num   { color: #2563eb; font-weight: 700; }
.port-tag   { font-size: 0.7em; color: #6b7280; margin-left: 0.3em; font-family: inherit; }
.st-listen  { color: #16a34a; font-weight: 600; }
.st-estab   { color: #6b7280; }
.st-other   { color: #9ca3af; }

/* ── Plus / launch button ── */
.btn-plus { background: #7c3aed; border-color: #7c3aed; color: #fff; font-weight: 700; }
.btn-plus:hover { background: #6d28d9 !important; border-color: #6d28d9 !important; }
.btn-plus.open  { background: #5b21b6; border-color: #5b21b6; }

/* ── Inline launch picker ── */
.lp-wrap {
    background: #faf5ff;
    border: 1.5px solid #ddd6fe;
    border-radius: 10px;
    padding: 0.85em 1em;
    margin-bottom: 0.85em;
    animation: lpSlide 0.13s ease;
}
@keyframes lpSlide {
    from { opacity: 0; transform: translateY(-4px); }
    to   { opacity: 1; transform: translateY(0); }
}
.lp-group-title {
    font-size: 0.68em;
    font-weight: 700;
    text-transform: uppercase;
    letter-spacing: 0.09em;
    color: #7c3aed;
    padding-bottom: 0.3em;
    border-bottom: 1px solid #ede9fe;
    margin: 0.65em 0 0.35em;
}
.lp-group-title:first-child { margin-top: 0; }
.lp-items { display: flex; flex-wrap: wrap; gap: 0.4em; }
.lp-item {
    display: inline-flex;
    align-items: center;
    gap: 0.4em;
    padding: 0.3em 0.7em;
    border-radius: 6px;
    font-size: 0.8em;
    font-weight: 600;
    color: #374151;
    text-decoration: none;
    border: 1px solid #dde1ea;
    background: #fff;
    transition: background 0.1s, border-color 0.1s;
    cursor: pointer;
    white-space: nowrap;
}
.lp-item:hover { background: #f5f3ff; border-color: #c4b5fd; color: #5b21b6; }
.lp-loading { color: #9ca3af; font-size: 0.82em; padding: 0.35em 0; }
`;

const OVERVIEW_TILES = [
    { label: 'Dashboard Host',  value: 'localhost:8000',    cls: '' },
    { label: 'Server Status',   value: '● Online',          cls: 'ok' },
    { label: 'Local Address',   value: '127.0.0.1',         cls: '' },
    { label: 'Protocol',        value: 'HTTP / FastAPI',    cls: '' },
    { label: 'Firewall',        value: '⚠ Local only',      cls: 'warn' },
    { label: 'API Docs',
      value: '<a href="/docs" target="_blank" rel="noopener noreferrer">OpenAPI /docs ↗</a>',
      cls: '' },
    { label: 'Python Runtime',  value: 'Uvicorn / Python 3', cls: '' },
    { label: 'WebSocket',       value: 'Not configured',    cls: 'warn' },
];

// ── Helpers ───────────────────────────────────────────────────────────────────

const KNOWN_PORTS = {
    22: 'SSH',  53: 'DNS',  80: 'HTTP',  443: 'HTTPS',
    3000: 'Node/Dev',  3306: 'MySQL',  3389: 'RDP',
    5432: 'PostgreSQL',  5601: 'Kibana',
    6379: 'Redis',  8000: 'CoolBox',  8080: 'Alt-HTTP',  8086: 'InfluxDB',
    9000: 'Dist-Master',  9092: 'Kafka',  9200: 'Elasticsearch',
    27017: 'MongoDB',
};

function makeBtn(text, cls = '') {
    const b = document.createElement('button');
    b.className = `btn ${cls}`;
    b.textContent = text;
    return b;
}

function makeEmpty(icon, msg, sub = '') {
    const d = document.createElement('div');
    d.className = 'empty';
    d.innerHTML = `<div class="empty-icon">${icon}</div>
                   <div>${msg}</div>
                   ${sub ? `<div class="empty-sub">${sub}</div>` : ''}`;
    return d;
}

// ── Component ─────────────────────────────────────────────────────────────────

class NetworkInfo extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // ── Sub-tab bar ────────────────────────────────────────────────────────
        const subBar = document.createElement('div');
        subBar.className = 'sub-bar';

        const overviewBtn = this._makeSubBtn('📊 Overview', true);
        const netBtn      = this._makeSubBtn('🌐 Network Services', false);
        subBar.appendChild(overviewBtn);
        subBar.appendChild(netBtn);
        shadow.appendChild(subBar);

        // ── Panels ─────────────────────────────────────────────────────────────
        const overviewPanel = document.createElement('div');
        overviewPanel.className = 'sub-panel active';
        this._buildOverview(overviewPanel);
        shadow.appendChild(overviewPanel);

        const netPanel = document.createElement('div');
        netPanel.className = 'sub-panel';
        shadow.appendChild(netPanel);

        this._panels = [
            { btn: overviewBtn, panel: overviewPanel, load: null },
            { btn: netBtn,      panel: netPanel,      load: () => this._loadNet(netPanel) },
        ];

        // ── Tab switching ──────────────────────────────────────────────────────
        this._panels.forEach((entry, idx) => {
            entry.btn.addEventListener('click', () => {
                this._panels.forEach(e => {
                    e.btn.setAttribute('aria-selected', 'false');
                    e.panel.classList.remove('active');
                });
                entry.btn.setAttribute('aria-selected', 'true');
                entry.panel.classList.add('active');
                if (entry.load) entry.load();
            });
        });
    }

    // ── Sub-tab button factory ─────────────────────────────────────────────────
    _makeSubBtn(label, active) {
        const b = document.createElement('button');
        b.className = 'sub-btn';
        b.textContent = label;
        b.setAttribute('aria-selected', active ? 'true' : 'false');
        return b;
    }

    // ── Overview tab ───────────────────────────────────────────────────────────
    _buildOverview(panel) {
        const grid = document.createElement('div');
        grid.className = 'ni-grid';
        for (const t of OVERVIEW_TILES) {
            const tile = document.createElement('div');
            tile.className = 'ni-tile';
            tile.innerHTML = `
                <div class="ni-label">${t.label}</div>
                <div class="ni-value ${t.cls}">${t.value}</div>`;
            grid.appendChild(tile);
        }
        panel.appendChild(grid);

        const note = document.createElement('p');
        note.className = 'ni-note';
        note.textContent = 'Network details are placeholder values — dynamic discovery coming soon.';
        panel.appendChild(note);
    }

    // ── Local Services tab ─────────────────────────────────────────────────────
    _loadLocal(panel) {
        if (panel._built) { this._refreshLocal(panel); return; }
        panel._built = true;

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        const refreshBtn = makeBtn('↻ Refresh');
        const plusBtn    = makeBtn('＋ Launch', 'btn-plus');
        const statusTxt  = document.createElement('span');
        statusTxt.className = 'status-txt';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(plusBtn);
        toolbar.appendChild(statusTxt);
        panel.appendChild(toolbar);

        // ── Launch picker (inline collapsible) ──
        const lpWrap = document.createElement('div');
        lpWrap.className = 'lp-wrap';
        lpWrap.style.display = 'none';
        lpWrap.innerHTML = '<div class="lp-loading">⟳ Loading…</div>';
        panel.appendChild(lpWrap);
        panel._lpWrap   = lpWrap;
        panel._lpLoaded = false;

        // ── Active Ports section ──
        const portsHdr = document.createElement('div');
        portsHdr.className = 'section-hdr';
        const portsTitle = document.createElement('span');
        portsTitle.textContent = '🔌 Active Ports';
        const portsCount = document.createElement('span');
        portsCount.className = 'section-hdr-count';
        portsHdr.appendChild(portsTitle);
        portsHdr.appendChild(portsCount);
        panel.appendChild(portsHdr);

        const portsWrap = document.createElement('div');
        portsWrap.className = 'ports-wrap';
        const portsTable = document.createElement('table');
        portsTable.className = 'ports-table';
        portsTable.innerHTML = `<thead><tr>
            <th>Port</th><th>Proto</th><th>Bound to</th><th>Status</th><th>Process</th><th>PID</th>
        </tr></thead>`;
        const portsTbody = document.createElement('tbody');
        portsTbody.innerHTML = '<tr><td colspan="6" class="lp-loading">Loading…</td></tr>';
        portsTable.appendChild(portsTbody);
        portsWrap.appendChild(portsTable);
        panel.appendChild(portsWrap);
        panel._portsTbody = portsTbody;
        panel._portsCount = portsCount;

        // ── Managed Services section ──
        const svcHdr = document.createElement('div');
        svcHdr.className = 'section-hdr';
        svcHdr.innerHTML = '<span>🖥️ Managed Services</span>';
        panel.appendChild(svcHdr);

        const grid = document.createElement('div');
        grid.className = 'svc-grid';
        panel.appendChild(grid);
        panel._grid      = grid;
        panel._statusTxt = statusTxt;

        // ── Listeners ──
        refreshBtn.addEventListener('click', () => this._refreshLocal(panel));

        plusBtn.addEventListener('click', () => {
            const isOpen = lpWrap.style.display !== 'none';
            if (isOpen) {
                lpWrap.style.display = 'none';
                plusBtn.classList.remove('open');
            } else {
                lpWrap.style.display = 'block';
                plusBtn.classList.add('open');
                if (!panel._lpLoaded) this._populateLaunchPicker(lpWrap, panel);
            }
        });

        this._refreshLocal(panel);
    }

    async _refreshLocal(panel) {
        this._refreshPorts(panel);
        panel._statusTxt.textContent = 'Loading…';
        try {
            const data = await fetch('/network/local-services').then(r => r.json());
            panel._grid.innerHTML = '';

            if (!data.services || !data.services.length) {
                panel._grid.appendChild(makeEmpty('🖥️', 'No local services defined.'));
                panel._statusTxt.textContent = '';
                return;
            }

            for (const svc of data.services) {
                panel._grid.appendChild(this._svcCard(svc, panel));
            }
            panel._statusTxt.textContent = `Updated ${new Date().toLocaleTimeString()}`;
        } catch (err) {
            panel._statusTxt.textContent = `Error: ${err.message}`;
        }
    }

    async _refreshPorts(panel) {
        if (!panel._portsTbody) return;
        try {
            const data = await fetch('/network/ports').then(r => r.json());
            const tbody = panel._portsTbody;
            tbody.innerHTML = '';

            if (!data.ports || !data.ports.length) {
                const tr = document.createElement('tr');
                tr.innerHTML = data.error
                    ? `<td colspan="6" class="lp-loading" style="color:#d97706">${data.error}</td>`
                    : `<td colspan="6" class="lp-loading">No active ports detected.</td>`;
                tbody.appendChild(tr);
                if (panel._portsCount) panel._portsCount.textContent = '';
                return;
            }

            for (const p of data.ports) {
                const tr = document.createElement('tr');
                const known = KNOWN_PORTS[p.port];
                if (known) tr.classList.add('port-known');
                const stCls = p.status === 'LISTEN'      ? 'st-listen'
                            : p.status === 'ESTABLISHED' ? 'st-estab'
                            : 'st-other';
                tr.innerHTML = `
                    <td><span class="port-num">${p.port}</span>${known ? `<span class="port-tag">${known}</span>` : ''}</td>
                    <td>${p.proto}</td>
                    <td>${p.host}</td>
                    <td class="${stCls}">${p.status || '—'}</td>
                    <td>${p.process || '—'}</td>
                    <td>${p.pid || '—'}</td>`;
                tbody.appendChild(tr);
            }
            if (panel._portsCount) panel._portsCount.textContent = `${data.ports.length} active`;
        } catch (err) {
            if (panel._portsTbody) {
                panel._portsTbody.innerHTML = `<tr><td colspan="6" class="lp-loading" style="color:#dc2626">Error: ${err.message}</td></tr>`;
            }
        }
    }

    async _populateLaunchPicker(wrap, panel) {
        panel._lpLoaded = true;
        wrap.innerHTML = '<div class="lp-loading">⟳ Loading…</div>';
        try {
            const [feData, mwData] = await Promise.all([
                fetch('/client-fe').then(r => r.json()),
                fetch('/middle-wear').then(r => r.json()),
            ]);
            wrap.innerHTML = '';

            const FE_ICONS = { client_facing_portal:'🏢', internal_login:'🔐', timesheets:'🕐', contracts:'📋', clients:'👥' };
            const MW_ICONS = { database_management:'🗄️', nginx_setup:'🌐', distributed_setup:'⚙️' };

            const addGroup = (title, items, route, icons) => {
                if (!items || !items.length) return;
                const gt = document.createElement('div');
                gt.className = 'lp-group-title';
                gt.textContent = title;
                wrap.appendChild(gt);
                const row = document.createElement('div');
                row.className = 'lp-items';
                for (const item of items) {
                    const f = item.folder;
                    const label = f.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
                    const a = document.createElement('a');
                    a.className = 'lp-item';
                    a.href = `/${route}/${f}`;
                    a.target = '_blank';
                    a.rel = 'noopener noreferrer';
                    a.innerHTML = `${icons[f] || '🌐'} ${label}`;
                    row.appendChild(a);
                }
                wrap.appendChild(row);
            };

            addGroup('🏢 Client FE Portals', feData.portals,  'client-portal', FE_ICONS);
            addGroup('🔧 Middleware Tools',  mwData.tools,    'middle-portal', MW_ICONS);

            if (!wrap.children.length) {
                wrap.innerHTML = '<div class="lp-loading">No items found — check that business_suite/ exists.</div>';
            }
        } catch (err) {
            wrap.innerHTML = `<div class="lp-loading" style="color:#dc2626">Error: ${err.message}</div>`;
        }
    }

    _svcCard(svc, panel) {
        const card = document.createElement('div');
        const dotCls = svc.running ? 'dot-green' : (svc.built ? 'dot-gray' : 'dot-amber');
        const badgeText = svc.running ? 'Running' : (svc.built ? 'Stopped' : 'Not Built');
        const badgeCls  = svc.running ? 'badge-run' : (svc.built ? 'badge-stop' : 'badge-nobuild');
        card.className  = `svc-card${svc.running ? ' running' : (!svc.built ? ' not-built' : '')}`;

        card.innerHTML = `
            <div class="svc-header">
                <div class="svc-dot ${dotCls}"></div>
                <div class="svc-label">${svc.label}</div>
                <span class="svc-badge ${badgeCls}">${badgeText}</span>
            </div>
            <div class="svc-desc">${svc.description}</div>
            <div class="svc-meta">exe: ${svc.exe}${svc.pid ? `  ·  pid: ${svc.pid}` : ''}</div>
        `;

        const actions = document.createElement('div');
        actions.className = 'svc-actions';

        const launchBtn = makeBtn('▶ Launch', 'btn-launch');
        launchBtn.disabled = svc.running || !svc.built;

        const stopBtn = makeBtn('■ Stop', 'btn-danger');
        stopBtn.disabled = !svc.running;

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
            setTimeout(() => this._refreshLocal(panel), 600);
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
            setTimeout(() => this._refreshLocal(panel), 600);
        });

        const buildLink = makeBtn('🔨 Build', 'btn-build');
        buildLink.title = 'Open Package Builder → Apps to build this executable';
        buildLink.addEventListener('click', () => {
            document.dispatchEvent(new CustomEvent('coolbox:navigate', {
                detail: { tab: 'libraries', subtab: 'apps' }
            }));
        });

        actions.appendChild(launchBtn);
        actions.appendChild(stopBtn);
        actions.appendChild(buildLink);
        card.appendChild(actions);
        return card;
    }

    // ── Network Services tab ───────────────────────────────────────────────────
    _loadNet(panel) {
        if (panel._built) { this._refreshNet(panel); return; }
        panel._built = true;

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        const refreshBtn = makeBtn('↻ Refresh');
        const statusTxt  = document.createElement('span');
        statusTxt.className = 'status-txt';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(statusTxt);
        panel.appendChild(toolbar);

        // Hint linking to Local Services
        const hint = document.createElement('p');
        hint.style.cssText = 'font-size:0.78em;color:#6b7280;margin-bottom:0.85em;';
        hint.innerHTML = 'Reflects the <strong>ServiceRegistry</strong> state of running '
            + '<code>distribution_tag</code> services. Launch them from '
            + '<strong>Package Builder → 🔧 Services</strong> to populate this view.';
        panel.appendChild(hint);

        const body = document.createElement('div');
        panel.appendChild(body);
        panel._body      = body;
        panel._statusTxt = statusTxt;

        refreshBtn.addEventListener('click', () => this._refreshNet(panel));
        this._refreshNet(panel);
    }

    async _refreshNet(panel) {
        panel._statusTxt.textContent = 'Loading…';
        try {
            const data = await fetch('/network/network-services').then(r => r.json());
            panel._body.innerHTML = '';

            if (!data.services || !data.services.length) {
                panel._body.appendChild(makeEmpty(
                    '🌐',
                    'No services registered.',
                    'Start distribution_tag_master or distribution_tag_worker from Local Services.'
                ));
                panel._statusTxt.textContent = '';
                return;
            }

            const table = document.createElement('table');
            table.className = 'ns-table';
            table.innerHTML = `
                <thead>
                    <tr>
                        <th>Service</th>
                        <th>Instance</th>
                        <th>Host : Port</th>
                        <th>Metadata</th>
                        <th>Health</th>
                    </tr>
                </thead>`;
            const tbody = document.createElement('tbody');
            for (const s of data.services) {
                const tr = document.createElement('tr');
                const healthTxt = s.healthy ? '● Healthy' : '○ Unhealthy';
                const healthCls = s.healthy ? 'health-ok' : 'health-bad';
                tr.innerHTML = `
                    <td>${s.service_name}</td>
                    <td>${s.instance_id}</td>
                    <td>${s.host}:${s.port || '—'}</td>
                    <td>${s.metadata || '—'}</td>
                    <td class="${healthCls}">${healthTxt}</td>`;
                tbody.appendChild(tr);
            }
            table.appendChild(tbody);
            panel._body.appendChild(table);
            panel._statusTxt.textContent = `Updated ${new Date().toLocaleTimeString()}`;
        } catch (err) {
            panel._statusTxt.textContent = `Error: ${err.message}`;
        }
    }
}

customElements.define('network-info', NetworkInfo);

