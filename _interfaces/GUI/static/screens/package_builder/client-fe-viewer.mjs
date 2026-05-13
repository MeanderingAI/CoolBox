/**
 * <client-fe-viewer>
 * Package Builder sub-tab: shows portals from client_fe/ (🌐 Portals) and
 * middleware tools from business_suite/middle_wear/ (🔧 Middleware) as inner sub-tabs.
 * Endpoint: GET /client-fe  →  { portals: [{ folder, title, description, has_index }] }
 *           GET /client-portal/{folder}  →  serves client_fe/{folder}/index.html
 */
import './middle-wear-viewer.mjs';

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

/* ── Portal grid ── */
.portal-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 1em;
    margin-bottom: 1em;
}

/* ── Portal card ── */
.portal-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1.1em 1.2em;
    display: flex;
    flex-direction: column;
    gap: 0.5em;
    transition: border-color 0.15s, box-shadow 0.15s;
}
.portal-card:hover {
    border-color: #2563eb;
    box-shadow: 0 3px 14px rgba(37,99,235,0.09);
}
.portal-card.active {
    border-color: #2563eb;
    background: #eff6ff;
}

/* ── Card header ── */
.pc-header { display: flex; align-items: center; gap: 0.6em; }
.pc-icon   { font-size: 1.5em; }
.pc-title  { font-size: 0.95em; font-weight: 700; color: #1e293b; flex: 1; }
.pc-folder {
    font-size: 0.7em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #94a3b8;
}
.pc-desc { font-size: 0.8em; color: #64748b; line-height: 1.5; }

/* ── Card actions ── */
.pc-actions { display: flex; gap: 0.4em; flex-wrap: wrap; margin-top: 0.25em; }

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
    background: #2563eb;
    color: #fff;
    border-color: #2563eb;
}
.ac-btn.open:hover { background: #1d4ed8; }
.ac-btn.open.active { background: #1e40af; }
.ac-btn.ext {
    background: #f8fafc;
    border-color: #cbd5e1;
    color: #475569;
}
.ac-btn.ext:hover { background: #f1f5f9; }

/* ── Iframe viewer ── */
.viewer-wrap {
    border: 1.5px solid #bfdbfe;
    border-radius: 10px;
    overflow: hidden;
    animation: slideIn 0.18s ease;
}
@keyframes slideIn {
    from { opacity: 0; transform: translateY(-6px); }
    to   { opacity: 1; transform: translateY(0); }
}
.viewer-header {
    background: #2563eb;
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

/* ── Inner sub-tab bar ── */
.inner-bar {
    display: flex;
    gap: 0.35em;
    padding: 0.6em 0 0;
    border-bottom: 1px solid #dde1ea;
    margin-bottom: 0.75em;
}
.inner-btn {
    padding: 0.28em 0.85em;
    font-size: 0.8em;
    font-family: inherit;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 4px 4px 0 0;
    border-bottom: none;
    cursor: pointer;
    color: #555;
    margin-bottom: -1px;
    transition: background 0.12s, color 0.12s;
}
.inner-btn:hover { background: #e8eaf0; color: #222; }
.inner-btn[aria-selected="true"] {
    background: #fff;
    border-color: #2563eb;
    color: #2563eb;
    font-weight: 600;
    border-bottom: 1px solid #fff;
}
.inner-panel { display: none; }
.inner-panel.active { display: block; }

/* ── Employee Chart section ── */
.emp-section-label {
    padding: 0.6em 0 0.5em;
    font-size: 0.78em;
    color: #64748b;
    font-weight: 600;
    letter-spacing: 0.05em;
    text-transform: uppercase;
}
`;

const FOLDER_ICONS = {
    client_facing_portal: '🏢',
    internal_login:       '🔐',
    timesheets:           '🕐',
    contracts:            '📋',
    clients:              '👥',
    office_sweet:         '📂',
    chorus:               '📖',
    mermaid:              '✏️',
};

const FOLDER_DESCRIPTIONS = {
    client_facing_portal: 'Public-facing company overview for Meandering LLC — services, case studies, and contact.',
    internal_login:       'Internal login and account creation page with password strength meter and SSO placeholder.',
    timesheets:           'Weekly timesheet entry with project/task rows, daily hour inputs, and approval workflow.',
    contracts:            'Contract management — view, filter, and track client agreements and delivery progress.',
    clients:              'Client CRM — contacts, revenue, contract counts, and a slide-out detail drawer.',
    office_sweet:         'Cloud file storage and collaboration. Share documents, spreadsheets and media with clients in a Google Drive-style workspace.',
    chorus:               'Team knowledge base and wiki. Write structured articles, link pages together and organise content by space or tag.',
    mermaid:              'Visual diagram drawing tool. Create flowcharts, sequence diagrams, org charts and architecture maps with an interactive canvas.',
};

/** Portals shown in the Office Tools tab. */
const OFFICE_TOOL_FOLDERS = new Set(['office_sweet', 'chorus', 'mermaid']);

/** Portals that belong exclusively to the Employee Chart tab. */
const EMPLOYEE_PORTALS = new Set(['contracts', 'clients', 'internal_login', 'timesheets']);

function iconFor(folder) {
    return FOLDER_ICONS[folder] ?? '🌐';
}

function titleFor(folder) {
    return folder.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase());
}

class ClientFeViewer extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        this._shadow = shadow;
        this._activeFolder = null;
        this._cards = new Map();
        this._middlewareLoaded = false;
        this._employeeGrid = null;
        this._officeGrid = null;

        // ── Inner sub-tab bar ──
        const innerBar = document.createElement('div');
        innerBar.className = 'inner-bar';
        const portalsBtn       = this._makeInnerBtn('🌐 Portals',        true);
        const officeToolsBtn   = this._makeInnerBtn('🏢 Office Tools',   false);
        const middlewareBtn    = this._makeInnerBtn('🔧 Middleware',     false);
        const employeeChartBtn = this._makeInnerBtn('👥 Employee Chart', false);
        innerBar.appendChild(portalsBtn);
        innerBar.appendChild(officeToolsBtn);
        innerBar.appendChild(middlewareBtn);
        innerBar.appendChild(employeeChartBtn);
        shadow.appendChild(innerBar);

        // ── Portals section ──
        const portalsSection = document.createElement('div');
        portalsSection.className = 'inner-panel active';

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'btn';
        refreshBtn.textContent = '↻ Refresh';
        this._statusEl = document.createElement('span');
        this._statusEl.className = 'status';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(this._statusEl);
        portalsSection.appendChild(toolbar);

        // Grid
        this._grid = document.createElement('div');
        this._grid.className = 'portal-grid';
        portalsSection.appendChild(this._grid);

        shadow.appendChild(portalsSection);

        // ── Office Tools section ──
        const officeToolsSection = document.createElement('div');
        officeToolsSection.className = 'inner-panel';
        const officeLabel = document.createElement('div');
        officeLabel.className = 'emp-section-label';
        officeLabel.textContent = 'Office & Productivity Tools';
        officeToolsSection.appendChild(officeLabel);
        this._officeGrid = document.createElement('div');
        this._officeGrid.className = 'portal-grid';
        officeToolsSection.appendChild(this._officeGrid);
        shadow.appendChild(officeToolsSection);

        // ── Middleware section ──
        const middlewareSection = document.createElement('div');
        middlewareSection.className = 'inner-panel';
        shadow.appendChild(middlewareSection);

        // ── Employee Chart section ──
        const employeeChartSection = document.createElement('div');
        employeeChartSection.className = 'inner-panel';
        const empLabel = document.createElement('div');
        empLabel.className = 'emp-section-label';
        empLabel.textContent = 'Employee Portals';
        employeeChartSection.appendChild(empLabel);
        this._employeeGrid = document.createElement('div');
        this._employeeGrid.className = 'portal-grid';
        employeeChartSection.appendChild(this._employeeGrid);
        shadow.appendChild(employeeChartSection);

        // ── Viewer (appended to shadow root, shared across portal cards) ──
        this._viewerWrap = null;

        refreshBtn.addEventListener('click', () => this._load());
        this._load();

        // Inner sub-tab switching
        const allInner    = [portalsBtn, officeToolsBtn, middlewareBtn, employeeChartBtn];
        const allSections = [portalsSection, officeToolsSection, middlewareSection, employeeChartSection];
        const activateInner = (btn, section) => {
            allInner.forEach(b => b.setAttribute('aria-selected', b === btn ? 'true' : 'false'));
            allSections.forEach(s => s.classList.toggle('active', s === section));
            if (section === middlewareSection && !this._middlewareLoaded) {
                this._middlewareLoaded = true;
                middlewareSection.appendChild(document.createElement('middle-wear-viewer'));
            }
            this._closeViewer();
        };
        portalsBtn.addEventListener('click',       () => activateInner(portalsBtn,       portalsSection));
        officeToolsBtn.addEventListener('click',   () => activateInner(officeToolsBtn,   officeToolsSection));
        middlewareBtn.addEventListener('click',    () => activateInner(middlewareBtn,    middlewareSection));
        employeeChartBtn.addEventListener('click', () => activateInner(employeeChartBtn, employeeChartSection));
    }

    _makeInnerBtn(label, active) {
        const btn = document.createElement('button');
        btn.className = 'inner-btn';
        btn.textContent = label;
        btn.setAttribute('aria-selected', active ? 'true' : 'false');
        return btn;
    }

    async _load() {
        this._statusEl.textContent = 'Loading…';
        this._grid.innerHTML = '';
        this._cards.clear();
        this._closeViewer();

        let data;
        try {
            const r = await fetch('/client-fe');
            data = await r.json();
        } catch (e) {
            this._grid.innerHTML = '';
            const err = document.createElement('div');
            err.className = 'loading';
            err.textContent = 'Failed to load client portals: ' + e.message;
            this._grid.appendChild(err);
            this._statusEl.textContent = '';
            return;
        }

        const publicPortals   = (data.portals || []).filter(p => !EMPLOYEE_PORTALS.has(p.folder) && !OFFICE_TOOL_FOLDERS.has(p.folder));
        const officePortals   = (data.portals || []).filter(p =>  OFFICE_TOOL_FOLDERS.has(p.folder));
        const employeePortals = (data.portals || []).filter(p =>  EMPLOYEE_PORTALS.has(p.folder));

        if (!publicPortals.length) {
            const empty = document.createElement('div');
            empty.className = 'empty';
            empty.innerHTML = `<div class="empty-icon">🌐</div>
                               <div>No portals found in client_fe/.</div>`;
            this._grid.appendChild(empty);
            this._statusEl.textContent = '';
        } else {
            for (const portal of publicPortals) {
                const card = this._makeCard(portal);
                this._cards.set(portal.folder, card);
                this._grid.appendChild(card);
            }
            this._statusEl.textContent = `${publicPortals.length} portal${publicPortals.length !== 1 ? 's' : ''}`;
        }
        this._populateOfficeGrid(officePortals);
        this._populateEmployeeGrid(employeePortals);
    }

    _populateOfficeGrid(portals) {
        if (!this._officeGrid) return;
        this._officeGrid.innerHTML = '';

        if (!portals.length) {
            const empty = document.createElement('div');
            empty.className = 'empty';
            empty.innerHTML = `<div class="empty-icon">🏢</div>
                               <div>No office tools found in client_fe/.</div>`;
            this._officeGrid.appendChild(empty);
            return;
        }

        for (const portal of portals) {
            const card = this._makeCard(portal);
            this._cards.set(portal.folder, card);
            this._officeGrid.appendChild(card);
        }
    }

    _makeCard(portal) {
        const card = document.createElement('div');
        card.className = 'portal-card';

        // Header
        const header = document.createElement('div');
        header.className = 'pc-header';
        header.innerHTML = `
            <span class="pc-icon">${iconFor(portal.folder)}</span>
            <div style="flex:1;min-width:0">
                <div class="pc-title">${this._esc(portal.title || titleFor(portal.folder))}</div>
                <div class="pc-folder">_interfaces/business_suite/client_fe/${this._esc(portal.folder)}</div>
            </div>`;
        card.appendChild(header);

        // Description — prefer portal.json meta, fall back to local map
        const descText = portal.description || FOLDER_DESCRIPTIONS[portal.folder] || null;
        if (descText) {
            const desc = document.createElement('div');
            desc.className = 'pc-desc';
            desc.textContent = descText;
            card.appendChild(desc);
        }

        // Actions
        const actions = document.createElement('div');
        actions.className = 'pc-actions';

        if (portal.has_index) {
            const openBtn = document.createElement('button');
            openBtn.className = 'ac-btn open';
            openBtn.textContent = '🖼 Open Portal';
            openBtn.addEventListener('click', () => {
                if (this._activeFolder === portal.folder) {
                    this._closeViewer();
                } else {
                    this._openViewer(portal);
                }
            });
            actions.appendChild(openBtn);

            const extLink = document.createElement('a');
            extLink.className = 'ac-btn ext';
            extLink.textContent = '↗ New tab';
            extLink.href = `/client-portal/${encodeURIComponent(portal.folder)}`;
            extLink.target = '_blank';
            extLink.rel = 'noopener noreferrer';
            actions.appendChild(extLink);
        }

        card.appendChild(actions);
        card._openBtn = actions.querySelector('.ac-btn.open');
        return card;
    }

    _populateEmployeeGrid(portals) {
        if (!this._employeeGrid) return;
        this._employeeGrid.innerHTML = '';

        if (!portals.length) {
            const empty = document.createElement('div');
            empty.className = 'empty';
            empty.innerHTML = `<div class="empty-icon">👥</div>
                               <div>No employee portals found in client_fe/.</div>`;
            this._employeeGrid.appendChild(empty);
            return;
        }

        for (const portal of portals) {
            const card = this._makeCard(portal);
            this._cards.set(portal.folder, card);
            this._employeeGrid.appendChild(card);
        }
    }

    _openViewer(portal) {
        // Deactivate previous card
        if (this._activeFolder) {
            const prev = this._cards.get(this._activeFolder);
            if (prev) {
                prev.classList.remove('active');
                if (prev._openBtn) prev._openBtn.classList.remove('active');
                if (prev._openBtn) prev._openBtn.textContent = '🖼 Open Portal';
            }
        }

        this._activeFolder = portal.folder;
        const card = this._cards.get(portal.folder);
        if (card) {
            card.classList.add('active');
            if (card._openBtn) {
                card._openBtn.classList.add('active');
                card._openBtn.textContent = '✕ Close';
            }
        }

        // Remove existing viewer
        if (this._viewerWrap) this._viewerWrap.remove();

        const wrap = document.createElement('div');
        wrap.className = 'viewer-wrap';

        const header = document.createElement('div');
        header.className = 'viewer-header';
        header.innerHTML = `<span class="vh-title">${iconFor(portal.folder)} ${this._esc(portal.title || titleFor(portal.folder))}</span>`;

        const closeBtn = document.createElement('button');
        closeBtn.className = 'viewer-close';
        closeBtn.textContent = '✕ Close';
        closeBtn.addEventListener('click', () => this._closeViewer());
        header.appendChild(closeBtn);

        const iframe = document.createElement('iframe');
        iframe.src = `/client-portal/${encodeURIComponent(portal.folder)}`;
        iframe.title = portal.title || titleFor(portal.folder);
        iframe.setAttribute('sandbox', 'allow-same-origin allow-scripts allow-forms');

        wrap.appendChild(header);
        wrap.appendChild(iframe);

        this._shadow.appendChild(wrap);
        this._viewerWrap = wrap;

        // Scroll into view
        setTimeout(() => wrap.scrollIntoView({ behavior: 'smooth', block: 'start' }), 80);
    }

    _closeViewer() {
        if (this._activeFolder) {
            const card = this._cards.get(this._activeFolder);
            if (card) {
                card.classList.remove('active');
                if (card._openBtn) {
                    card._openBtn.classList.remove('active');
                    card._openBtn.textContent = '🖼 Open Portal';
                }
            }
        }
        this._activeFolder = null;
        if (this._viewerWrap) {
            this._viewerWrap.remove();
            this._viewerWrap = null;
        }
    }

    _esc(s) {
        return String(s ?? '')
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }
}

customElements.define('client-fe-viewer', ClientFeViewer);
