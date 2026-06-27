/**
 * <app-launcher>
 * Lists all apps from apps/, each with:
 *   - cmake apps: Build + Launch buttons per exe target
 *   - python apps: Launch (run script) button per .py script
 *   - Info link (opens app_page.html) when has_page is true
 * Endpoints: GET /apps, POST /apps/build, POST /apps/launch, POST /apps/run-script
 */

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Grid of app cards ───────────────────────────────── */
.app-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
    gap: 1em;
    padding: 0.5em 0;
}

/* ── App card ────────────────────────────────────────── */
.app-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1em 1.1em 0.8em;
    display: flex;
    flex-direction: column;
    gap: 0.6em;
    transition: box-shadow 0.15s, border-color 0.15s;
}
.app-card:hover {
    border-color: #7c3aed;
    box-shadow: 0 3px 12px rgba(124,58,237,0.10);
}

/* ── Header row ──────────────────────────────────────── */
.ac-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
}
.ac-icon { font-size: 1.4em; }
.ac-name {
    font-size: 1em;
    font-weight: 700;
    color: #1f2937;
    flex: 1;
}
.ac-folder {
    font-size: 0.7em;
    color: #9ca3af;
    font-family: 'Cascadia Code', 'Consolas', monospace;
}

/* ── Type badge ──────────────────────────────────────── */
.ac-type {
    display: inline-block;
    font-size: 0.68em;
    font-weight: 700;
    padding: 0.1em 0.5em;
    border-radius: 3px;
    background: #ede9fe;
    color: #5b21b6;
    border: 1px solid #ddd6fe;
}
.ac-type.cmake  { background: #eff6ff; color: #1d4ed8; border-color: #bfdbfe; }
.ac-type.python { background: #fef3c7; color: #92400e; border-color: #fde68a; }

/* ── Targets / scripts label ─────────────────────────── */
.ac-targets-label {
    font-size: 0.7em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6b7280;
}

/* ── Action rows ─────────────────────────────────────── */
.ac-target-row {
    display: flex;
    flex-direction: column;
    gap: 0.35em;
    background: #f8faff;
    border: 1px solid #e5e9f0;
    border-radius: 5px;
    padding: 0.4em 0.6em;
}
.ac-target-name {
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.78em;
    font-weight: 600;
    color: #374151;
}
.ac-target-actions {
    display: flex;
    align-items: center;
    gap: 0.4em;
    flex-wrap: wrap;
}

/* ── Buttons ─────────────────────────────────────────── */
.ac-btn {
    padding: 0.25em 0.75em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 4px;
    cursor: pointer;
    border: 1px solid transparent;
    font-weight: 600;
    transition: background 0.12s, opacity 0.12s;
    text-decoration: none;
    display: inline-block;
}
.ac-btn:disabled { opacity: 0.5; cursor: default; }

.ac-btn.build  { background: #0e639c; color: #fff; border-color: #0e639c; }
.ac-btn.build:hover:not(:disabled)  { background: #1177bb; }

.ac-btn.launch { background: #16a34a; color: #fff; border-color: #16a34a; }
.ac-btn.launch:hover:not(:disabled) { background: #15803d; }

.ac-btn.run    { background: #b45309; color: #fff; border-color: #b45309; }
.ac-btn.run:hover:not(:disabled)    { background: #92400e; }

.ac-btn.info   { background: #fafafa; border-color: #d1d5db; color: #6b7280; }
.ac-btn.info:hover { background: #f3f4f6; }

/* ── Status line ─────────────────────────────────────── */
.ac-status {
    font-size: 0.76em;
    color: #6b7280;
    margin-left: 0.3em;
    min-width: 0;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    flex: 2;
}
.ac-status.ok  { color: #16a34a; }
.ac-status.err { color: #dc2626; }

/* ── Collapsible console ─────────────────────────────── */
.ac-console {
    background: #1e1e2e;
    color: #d4d4d4;
    border-radius: 5px;
    padding: 0.5em 0.8em;
    font-size: 0.76em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    white-space: pre-wrap;
    word-break: break-all;
    max-height: 140px;
    overflow-y: auto;
    display: none;
    margin-top: 0.3em;
}
.ac-console.visible { display: block; }

/* ── Loading / error states ──────────────────────────── */
.al-loading { padding: 1.5em; text-align: center; color: #9ca3af; font-size: 0.88em; }
.al-error   { padding: 1em; color: #dc2626; font-size: 0.88em; }
`;

const APP_ICONS = {
    lsp:               '🔤',
    tutorial_editor:   '📝',
    distribution_tag:  '🏷️',
};

function iconFor(name) {
    return APP_ICONS[name] ?? '🛠️';
}

class AppLauncher extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        shadow.innerHTML = `<style>${STYLE}</style>`;

        this._root = shadow;
        this._load();
    }

    async _load() {
        const shadow = this._root;
        let existing = shadow.querySelector('.app-grid, .al-loading, .al-error');
        if (existing) existing.remove();

        const loading = document.createElement('div');
        loading.className = 'al-loading';
        loading.textContent = 'Loading apps…';
        shadow.appendChild(loading);

        let data;
        try {
            const r = await fetch('/apps');
            data = await r.json();
        } catch (e) {
            loading.className = 'al-error';
            loading.textContent = 'Failed to load apps: ' + e.message;
            return;
        }
        loading.remove();

        if (!data.apps || !data.apps.length) {
            const mt = document.createElement('div');
            mt.className = 'al-loading';
            mt.textContent = 'No apps found in apps/.';
            shadow.appendChild(mt);
            return;
        }

        const grid = document.createElement('div');
        grid.className = 'app-grid';

        for (const app of data.apps) {
            grid.appendChild(this._makeCard(app));
        }

        shadow.appendChild(grid);
    }

    _makeCard(app) {
        const card = document.createElement('div');
        card.className = 'app-card';

        // ── Header ──
        const header = document.createElement('div');
        header.className = 'ac-header';
        header.innerHTML = `
            <span class="ac-icon">${iconFor(app.folder)}</span>
            <div style="flex:1;min-width:0;">
                <div class="ac-name">${this._esc(app.name)}</div>
                <div class="ac-folder">apps/${this._esc(app.folder)}</div>
            </div>
        `;

        // Info link
        if (app.has_page) {
            const infoLink = document.createElement('a');
            infoLink.className = 'ac-btn info';
            infoLink.textContent = 'ℹ Info';
            infoLink.href = `/app-page/${encodeURIComponent(app.folder)}`;
            infoLink.target = '_blank';
            infoLink.rel = 'noopener noreferrer';
            header.appendChild(infoLink);
        }

        card.appendChild(header);

        // Type badge
        const typeBadge = document.createElement('span');
        typeBadge.className = `ac-type ${app.type}`;
        typeBadge.textContent = app.type === 'cmake' ? 'C++ / CMake' :
                                app.type === 'python' ? 'Python script' : 'App';
        card.appendChild(typeBadge);

        // ── cmake: exe targets ──
        if (app.type === 'cmake' && app.executables.length) {
            const lbl = document.createElement('div');
            lbl.className = 'ac-targets-label';
            lbl.textContent = 'Exe targets';
            card.appendChild(lbl);

            for (const exe of app.executables) {
                card.appendChild(this._makeCmakeRow(app, exe));
            }
        }

        // ── python: script rows ──
        if (app.scripts.length) {
            const lbl = document.createElement('div');
            lbl.className = 'ac-targets-label';
            lbl.textContent = app.type === 'cmake' ? 'Python scripts' : 'Scripts';
            card.appendChild(lbl);

            for (const script of app.scripts) {
                card.appendChild(this._makePythonRow(app, script));
            }
        }

        return card;
    }

    _makeCmakeRow(app, exe) {
        const row = document.createElement('div');
        row.className = 'ac-target-row';

        const name = document.createElement('span');
        name.className = 'ac-target-name';
        name.textContent = exe;

        const status = document.createElement('span');
        status.className = 'ac-status';

        const buildBtn = document.createElement('button');
        buildBtn.className = 'ac-btn build';
        buildBtn.textContent = '▶ Build';

        const launchBtn = document.createElement('button');
        launchBtn.className = 'ac-btn launch';
        launchBtn.textContent = '🚀 Launch';

        const console_ = document.createElement('pre');
        console_.className = 'ac-console';

        buildBtn.addEventListener('click', async () => {
            buildBtn.disabled = true;
            launchBtn.disabled = true;
            status.className = 'ac-status';
            status.textContent = 'Building…';
            console_.className = 'ac-console';
            console_.textContent = '';

            let res;
            try {
                const r = await fetch('/apps/build', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name: app.name, target: exe }),
                });
                res = await r.json();
            } catch (e) {
                status.className = 'ac-status err';
                status.textContent = 'Network error';
                buildBtn.disabled = false;
                return;
            }

            if (res.output) {
                console_.classList.add('visible');
                console_.textContent = res.output;
            }
            status.className = 'ac-status ' + (res.success ? 'ok' : 'err');
            status.textContent = res.success ? 'Build succeeded' : (res.error || 'Build failed');
            buildBtn.disabled = false;
            launchBtn.disabled = false;
        });

        launchBtn.addEventListener('click', async () => {
            launchBtn.disabled = true;
            status.className = 'ac-status';
            status.textContent = 'Launching…';

            let res;
            try {
                const r = await fetch('/apps/launch', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name: app.name, exe }),
                });
                res = await r.json();
            } catch (e) {
                status.className = 'ac-status err';
                status.textContent = 'Network error';
                launchBtn.disabled = false;
                return;
            }

            status.className = 'ac-status ' + (res.success ? 'ok' : 'err');
            status.textContent = res.output || (res.success ? 'Launched' : 'Failed');
            launchBtn.disabled = false;
        });

        const actions = document.createElement('div');
        actions.className = 'ac-target-actions';
        actions.append(buildBtn, launchBtn, status);
        row.append(name, actions);
        row.appendChild(console_);
        return row;
    }

    _makePythonRow(app, script) {
        const wrap = document.createElement('div');

        const row = document.createElement('div');
        row.className = 'ac-target-row';

        const name = document.createElement('span');
        name.className = 'ac-target-name';
        name.textContent = script;

        const status = document.createElement('span');
        status.className = 'ac-status';

        const runBtn = document.createElement('button');
        runBtn.className = 'ac-btn run';
        runBtn.textContent = '▶ Run';

        const console_ = document.createElement('pre');
        console_.className = 'ac-console';

        runBtn.addEventListener('click', async () => {
            runBtn.disabled = true;
            status.className = 'ac-status';
            status.textContent = 'Starting…';
            console_.className = 'ac-console';
            console_.textContent = '';

            let res;
            try {
                const r = await fetch('/apps/run-script', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ name: app.name, script }),
                });
                res = await r.json();
            } catch (e) {
                status.className = 'ac-status err';
                status.textContent = 'Network error';
                runBtn.disabled = false;
                return;
            }

            if (res.output) {
                console_.classList.add('visible');
                console_.textContent = res.output;
            }
            status.className = 'ac-status ' + (res.success ? 'ok' : 'err');
            status.textContent = res.output || (res.success ? 'Launched' : 'Failed');
            runBtn.disabled = false;
        });

        const actions = document.createElement('div');
        actions.className = 'ac-target-actions';
        actions.append(runBtn, status);
        row.append(name, actions);
        wrap.appendChild(row);
        wrap.appendChild(console_);
        return wrap;
    }

    _esc(s) {
        return String(s ?? '')
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }
}

customElements.define('app-launcher', AppLauncher);
