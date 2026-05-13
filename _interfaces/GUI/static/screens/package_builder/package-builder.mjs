// Custom element <package-builder>
// Sub-views: Legacy | Modern (card + search + deps) | 🧪 Tests | 🚀 Products | ️ Apps | 📖 Docs | 🏢 Client FE (portals + middleware) | 🧩 Extensions
import './groups-libraries.mjs';
import './doc-builder.mjs';
import './product-launcher.mjs';
import './app-launcher.mjs';
import './client-fe-viewer.mjs';
import './extension-builder.mjs';
import './demo-viewer.mjs';
import './service-viewer.mjs';
import '../test-runner.mjs';
import '../../components/editor-screen.mjs';
import { ApplicationState } from '../../automata/applicationState.mjs';

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Sub-tab bar ─────────────────────────────────────── */
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

/* ── Panels ──────────────────────────────────────────── */
.sub-panel { display: none; padding: 1em; }
.sub-panel.active { display: block; }
.sub-panel.demo-panel { padding: 0; }
.sub-panel.demo-panel.active { display: flex; flex-direction: column; height: calc(100vh - 110px); }

/* ── Search bar ──────────────────────────────────────── */
.search-wrap {
    display: flex;
    align-items: center;
    gap: 0.5em;
    margin-bottom: 1em;
}
.search-input {
    flex: 1;
    padding: 0.5em 0.8em;
    font-size: 0.9em;
    font-family: inherit;
    border: 1px solid #c5cad8;
    border-radius: 6px;
    outline: none;
    transition: border-color 0.15s, box-shadow 0.15s;
}
.search-input:focus {
    border-color: #0e639c;
    box-shadow: 0 0 0 2px rgba(14,99,156,0.15);
}
.search-clear {
    padding: 0.4em 0.7em;
    font-size: 0.8em;
    background: none;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    cursor: pointer;
    color: #666;
}
.search-clear:hover { background: #f0f0f0; }
.result-count { font-size: 0.78em; color: #888; white-space: nowrap; }

/* ── Group section ───────────────────────────────────── */
.group-section { margin-bottom: 1.5em; }
.group-heading {
    font-size: 0.72em;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: #6b7280;
    margin: 0 0 0.5em;
    padding: 0;
}

/* ── Card grid ───────────────────────────────────────── */
.card-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(160px, 1fr));
    gap: 0.6em;
}

/* ── Package card ─────────────────────────────────────── */
.pkg-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 8px;
    padding: 0.65em 0.8em;
    cursor: pointer;
    transition: border-color 0.15s, box-shadow 0.15s, background 0.15s;
    user-select: none;
    position: relative;
}
.pkg-card:hover {
    border-color: #0e639c;
    box-shadow: 0 2px 8px rgba(14,99,156,0.12);
}
.pkg-card[aria-selected="true"] {
    border-color: #0e639c;
    background: #f0f7ff;
    box-shadow: 0 3px 10px rgba(14,99,156,0.18);
}
.pkg-name {
    font-size: 0.88em;
    font-weight: 600;
    color: #1f2937;
    word-break: break-word;
}
.pkg-card[aria-selected="true"] .pkg-name { color: #0e639c; }

/* ── Deps panel (below the grid) ─────────────────────── */
.deps-panel {
    margin-top: 0.6em;
    background: #f8faff;
    border: 1px solid #c3d8f0;
    border-radius: 8px;
    padding: 0.8em 1em;
    animation: fadeIn 0.15s ease;
}
@keyframes fadeIn { from { opacity: 0; transform: translateY(-4px); } to { opacity: 1; transform: translateY(0); } }

.deps-title {
    font-size: 0.82em;
    font-weight: 700;
    color: #0e639c;
    margin: 0 0 0.5em;
}
.deps-loading { font-size: 0.8em; color: #888; }
.deps-empty   { font-size: 0.8em; color: #6b7280; font-style: italic; }
.deps-error   { font-size: 0.8em; color: #dc2626; }

/* ── Dep node rows ───────────────────────────────────── */
.dep-node {
    display: flex;
    align-items: baseline;
    gap: 0.4em;
    font-size: 0.82em;
    padding: 0.2em 0;
    border-bottom: 1px solid #e5edf8;
}
.dep-node:last-child { border-bottom: none; }
.dep-source {
    min-width: 9em;
    color: #374151;
    font-weight: 600;
    word-break: break-word;
}
.dep-arrow { color: #9ca3af; flex-shrink: 0; }
.dep-target { color: #1d4ed8; word-break: break-word; }
.dep-kind {
    font-size: 0.78em;
    padding: 0 0.4em;
    border-radius: 3px;
    flex-shrink: 0;
    font-weight: 600;
}
.dep-kind.PUBLIC    { background: #dcfce7; color: #166534; }
.dep-kind.PRIVATE   { background: #fef9c3; color: #854d0e; }
.dep-kind.INTERFACE { background: #e0e7ff; color: #3730a3; }

/* ── No results ──────────────────────────────────────── */
.no-results { font-size: 0.9em; color: #9ca3af; text-align: center; padding: 2em 0; }

/* ── Editor screen (file browser) ───────────────────── */
.editor-wrap {
    margin-top: 0.6em;
    border-radius: 6px;
    overflow: hidden;
    height: 380px;
    animation: fadeIn 0.15s ease;
}
editor-screen {
    display: flex;
    flex-direction: column;
    height: 100%;
}
`;

class PackageBuilder extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Sub-tabs
        const subBar = document.createElement('div');
        subBar.className = 'sub-bar';

        const legacyBtn      = this._makeSubBtn('Legacy', true);
        const modernBtn      = this._makeSubBtn('Modern ✦', false);
        const testsBtn       = this._makeSubBtn('🧪 Tests', false);
        const productsBtn    = this._makeSubBtn('🚀 Products', false);
        const appsBtn        = this._makeSubBtn('🛠️ Apps', false);
        const docsBtn        = this._makeSubBtn('📖 Docs', false);
        const clientFeBtn    = this._makeSubBtn('🏢 Client FE', false);
        const extensionsBtn  = this._makeSubBtn('🧩 Extensions', false);
        const demoBtn        = this._makeSubBtn('🎬 Demo', false);
        const servicesBtn    = this._makeSubBtn('🔧 Services', false);
        subBar.appendChild(legacyBtn);
        subBar.appendChild(modernBtn);
        subBar.appendChild(testsBtn);
        subBar.appendChild(productsBtn);
        subBar.appendChild(appsBtn);
        subBar.appendChild(docsBtn);
        subBar.appendChild(clientFeBtn);
        subBar.appendChild(extensionsBtn);
        subBar.appendChild(demoBtn);
        subBar.appendChild(servicesBtn);
        shadow.appendChild(subBar);

        // Legacy panel
        const legacyPanel = document.createElement('div');
        legacyPanel.className = 'sub-panel active';
        const gl = document.createElement('groups-libraries');
        legacyPanel.appendChild(gl);
        shadow.appendChild(legacyPanel);

        // Modern panel
        const modernPanel = document.createElement('div');
        modernPanel.className = 'sub-panel';
        modernPanel.textContent = 'Loading…';
        shadow.appendChild(modernPanel);

        // Tests panel
        const testsPanel = document.createElement('div');
        testsPanel.className = 'sub-panel';
        shadow.appendChild(testsPanel);

        // Products panel
        const productsPanel = document.createElement('div');
        productsPanel.className = 'sub-panel';
        shadow.appendChild(productsPanel);

        // Apps panel
        const appsPanel = document.createElement('div');
        appsPanel.className = 'sub-panel';
        shadow.appendChild(appsPanel);

        // Docs panel
        const docsPanel = document.createElement('div');
        docsPanel.className = 'sub-panel';
        shadow.appendChild(docsPanel);

        // Client FE panel (portals + middleware inner sub-tabs)
        const clientFePanel = document.createElement('div');
        clientFePanel.className = 'sub-panel';
        shadow.appendChild(clientFePanel);

        // Extensions panel
        const extensionsPanel = document.createElement('div');
        extensionsPanel.className = 'sub-panel';
        shadow.appendChild(extensionsPanel);

        // Demo panel
        const demoPanel = document.createElement('div');
        demoPanel.className = 'sub-panel demo-panel';
        shadow.appendChild(demoPanel);

        // Services panel
        const servicesPanel = document.createElement('div');
        servicesPanel.className = 'sub-panel';
        shadow.appendChild(servicesPanel);

        this._shadow = shadow;
        this._modernPanel = modernPanel;
        this._modernLoaded = false;
        this._testsLoaded = false;
        this._productsLoaded = false;
        this._appsLoaded = false;
        this._docsLoaded = false;
        this._clientFeLoaded = false;
        this._extensionsLoaded = false;
        this._demoLoaded = false;
        this._servicesLoaded = false;
        this._allGroups = [];
        this._selectedCard = null;
        this._depsPanel = null;

        const allBtns   = [legacyBtn, modernBtn, testsBtn, productsBtn, appsBtn, docsBtn, clientFeBtn, extensionsBtn, demoBtn, servicesBtn];
        const allPanels = [legacyPanel, modernPanel, testsPanel, productsPanel, appsPanel, docsPanel, clientFePanel, extensionsPanel, demoPanel, servicesPanel];

        const activate = (activeBtn, activePanel) => {
            allBtns.forEach(b => b.setAttribute('aria-selected', b === activeBtn ? 'true' : 'false'));
            allPanels.forEach(p => p.classList.toggle('active', p === activePanel));
            if (activePanel === modernPanel && !this._modernLoaded) {
                this._modernLoaded = true;
                this._renderModern();
            }
            if (activePanel === testsPanel && !this._testsLoaded) {
                this._testsLoaded = true;
                testsPanel.appendChild(document.createElement('test-runner'));
            }
            if (activePanel === productsPanel && !this._productsLoaded) {
                this._productsLoaded = true;
                productsPanel.appendChild(document.createElement('product-launcher'));
            }
            if (activePanel === appsPanel && !this._appsLoaded) {
                this._appsLoaded = true;
                appsPanel.appendChild(document.createElement('app-launcher'));
            }
            if (activePanel === docsPanel && !this._docsLoaded) {
                this._docsLoaded = true;
                docsPanel.appendChild(document.createElement('doc-builder'));
            }
            if (activePanel === clientFePanel && !this._clientFeLoaded) {
                this._clientFeLoaded = true;
                clientFePanel.appendChild(document.createElement('client-fe-viewer'));
            }
            if (activePanel === extensionsPanel && !this._extensionsLoaded) {
                this._extensionsLoaded = true;
                extensionsPanel.appendChild(document.createElement('extension-builder'));
            }
            if (activePanel === demoPanel && !this._demoLoaded) {
                this._demoLoaded = true;
                demoPanel.appendChild(document.createElement('demo-viewer'));
            }
            if (activePanel === servicesPanel && !this._servicesLoaded) {
                this._servicesLoaded = true;
                servicesPanel.appendChild(document.createElement('service-viewer'));
            }
        };

        legacyBtn.addEventListener('click',      () => activate(legacyBtn, legacyPanel));
        modernBtn.addEventListener('click',      () => activate(modernBtn, modernPanel));
        testsBtn.addEventListener('click',       () => activate(testsBtn, testsPanel));
        productsBtn.addEventListener('click',    () => activate(productsBtn, productsPanel));
        appsBtn.addEventListener('click',        () => activate(appsBtn, appsPanel));
        docsBtn.addEventListener('click',        () => activate(docsBtn, docsPanel));
        clientFeBtn.addEventListener('click',    () => activate(clientFeBtn, clientFePanel));
        extensionsBtn.addEventListener('click',  () => activate(extensionsBtn, extensionsPanel));
        demoBtn.addEventListener('click',        () => activate(demoBtn, demoPanel));
        servicesBtn.addEventListener('click',    () => activate(servicesBtn, servicesPanel));

        // Listen for cross-component subtab navigation (e.g. from Network → Build button)
        document.addEventListener('coolbox:subtab', (e) => {
            const { subtab } = e.detail || {};
            if (subtab === 'apps')        activate(appsBtn, appsPanel);
            if (subtab === 'client-fe')   activate(clientFeBtn, clientFePanel);
            if (subtab === 'extensions')  activate(extensionsBtn, extensionsPanel);
            if (subtab === 'services')    activate(servicesBtn, servicesPanel);
        });
    }

    _makeSubBtn(label, selected) {
        const btn = document.createElement('button');
        btn.className = 'sub-btn';
        btn.textContent = label;
        btn.setAttribute('aria-selected', selected ? 'true' : 'false');
        return btn;
    }

    async _renderModern() {
        const panel = this._modernPanel;
        try {
            const data = await ApplicationState.cacheFetch(ApplicationState.DASHBOARD_URL);
            this._allGroups = data.groups || [];
        } catch (e) {
            panel.textContent = `Failed to load: ${e}`;
            return;
        }

        panel.innerHTML = '';

        // Search bar
        const searchWrap = document.createElement('div');
        searchWrap.className = 'search-wrap';

        const searchInput = document.createElement('input');
        searchInput.type = 'search';
        searchInput.className = 'search-input';
        searchInput.placeholder = 'Filter by group, package or library…';
        searchInput.setAttribute('aria-label', 'Search packages');

        const clearBtn = document.createElement('button');
        clearBtn.className = 'search-clear';
        clearBtn.textContent = '✕';
        clearBtn.title = 'Clear search';

        const countSpan = document.createElement('span');
        countSpan.className = 'result-count';

        searchWrap.appendChild(searchInput);
        searchWrap.appendChild(clearBtn);
        searchWrap.appendChild(countSpan);
        panel.appendChild(searchWrap);

        // Content area (cards grid)
        const content = document.createElement('div');
        panel.appendChild(content);

        const render = (q) => this._renderCards(content, q, countSpan);
        render('');

        searchInput.addEventListener('input', () => render(searchInput.value));
        clearBtn.addEventListener('click', () => { searchInput.value = ''; render(''); searchInput.focus(); });
    }

    _renderCards(container, query, countSpan) {
        // Deselect any open card/deps when re-rendering
        this._selectedCard = null;
        this._depsPanel = null;

        container.innerHTML = '';
        const q = query.trim().toLowerCase();
        let totalShown = 0;

        const filteredGroups = this._allGroups.map(group => {
            const groupMatch = !q || group.name.toLowerCase().includes(q);
            const libs = group.libs.filter(lib =>
                groupMatch || lib.toLowerCase().includes(q)
            );
            return { ...group, libs };
        }).filter(g => g.libs.length > 0);

        if (!filteredGroups.length) {
            const msg = document.createElement('div');
            msg.className = 'no-results';
            msg.textContent = `No packages match "${query}"`;
            container.appendChild(msg);
            countSpan.textContent = '0 packages';
            return;
        }

        filteredGroups.forEach(group => {
            const section = document.createElement('div');
            section.className = 'group-section';

            const heading = document.createElement('p');
            heading.className = 'group-heading';
            heading.textContent = group.name;
            section.appendChild(heading);

            const grid = document.createElement('div');
            grid.className = 'card-grid';

            group.libs.forEach(lib => {
                const card = document.createElement('div');
                card.className = 'pkg-card';
                card.setAttribute('role', 'button');
                card.setAttribute('aria-selected', 'false');
                card.tabIndex = 0;

                const name = document.createElement('div');
                name.className = 'pkg-name';
                // Highlight matched text
                name.innerHTML = q ? this._highlight(lib, q) : this._esc(lib);
                card.appendChild(name);

                const onClick = () => this._selectCard(card, lib, group.name, section);
                card.addEventListener('click', onClick);
                card.addEventListener('keydown', e => { if (e.key === 'Enter' || e.key === ' ') onClick(); });

                grid.appendChild(card);
                totalShown++;
            });

            section.appendChild(grid);
            container.appendChild(section);
        });

        countSpan.textContent = `${totalShown} package${totalShown !== 1 ? 's' : ''}`;
    }

    _selectCard(card, lib, groupName, section) {
        // Toggle off if already selected
        if (this._selectedCard === card) {
            card.setAttribute('aria-selected', 'false');
            this._depsPanel?.remove();
            this._editorWrap?.remove();
            this._selectedCard = null;
            this._depsPanel = null;
            this._editorWrap = null;
            return;
        }

        // Deselect previous
        this._selectedCard?.setAttribute('aria-selected', 'false');
        this._depsPanel?.remove();
        this._editorWrap?.remove();
        this._editorWrap = null;

        card.setAttribute('aria-selected', 'true');
        this._selectedCard = card;

        // Insert deps panel after the grid, inside the group section
        const depsPanel = document.createElement('div');
        depsPanel.className = 'deps-panel';
        this._depsPanel = depsPanel;

        const title = document.createElement('div');
        title.className = 'deps-title';
        title.textContent = `Dependencies — ${groupName} / ${lib}`;
        depsPanel.appendChild(title);

        const body = document.createElement('div');
        body.className = 'deps-loading';
        body.textContent = 'Loading…';
        depsPanel.appendChild(body);

        section.appendChild(depsPanel);

        // ── Editor screen (library files browser) ──
        const editorWrap = document.createElement('div');
        editorWrap.className = 'editor-wrap';
        const editorEl = document.createElement('editor-screen');
        editorEl.setAttribute('group', groupName);
        editorEl.setAttribute('lib', lib);
        editorWrap.appendChild(editorEl);
        section.appendChild(editorWrap);
        this._editorWrap = editorWrap;

        // Fetch deps
        fetch(`/deps/${encodeURIComponent(lib)}`)
            .then(r => r.json())
            .then(result => {
                body.innerHTML = '';
                if (!result.success || !result.edges?.length) {
                    body.className = 'deps-empty';
                    body.textContent = result.success
                        ? `${lib} has no declared external dependencies.`
                        : (result.output || 'No dependency data found.');
                    return;
                }
                result.edges.forEach(e => {
                    const row = document.createElement('div');
                    row.className = 'dep-node';

                    const src = document.createElement('span');
                    src.className = 'dep-source';
                    src.textContent = e.from;

                    const arrow = document.createElement('span');
                    arrow.className = 'dep-arrow';
                    arrow.textContent = '→';

                    const tgt = document.createElement('span');
                    tgt.className = 'dep-target';
                    tgt.textContent = e.to;

                    const kind = document.createElement('span');
                    kind.className = `dep-kind ${e.kind}`;
                    kind.textContent = e.kind;

                    row.appendChild(src);
                    row.appendChild(arrow);
                    row.appendChild(tgt);
                    row.appendChild(kind);
                    body.appendChild(row);
                });
            })
            .catch(err => {
                body.className = 'deps-error';
                body.textContent = `Error: ${err}`;
            });
    }

    _highlight(text, q) {
        const idx = text.toLowerCase().indexOf(q);
        if (idx === -1) return this._esc(text);
        return this._esc(text.slice(0, idx))
            + `<mark style="background:#fef08a;border-radius:2px;">${this._esc(text.slice(idx, idx + q.length))}</mark>`
            + this._esc(text.slice(idx + q.length));
    }

    _esc(s) {
        return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');
    }
}

customElements.define('package-builder', PackageBuilder);
