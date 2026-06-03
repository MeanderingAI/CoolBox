/**
 * <extension-builder>
 * Package Builder sub-tab: shows binding packages from _deliverables/libraries/bindings/.
 * Each binding has a Build button that calls build_extensions.py.
 * Endpoint: GET  /extensions          → { bindings: [...] }
 *           GET  /extensions/tools     → { tools: [{ tool, label, installed }] }
 *           POST /extensions/install   → { tool: "go" }  (streaming)
 *           POST /extensions/build     → { binding: "python_bindings" }  (streaming)
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

/* ── Language section ── */
.lang-section {
    margin-bottom: 1.2em;
}
.lang-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.45em 0.7em;
    background: #f1f5f9;
    border: 1.5px solid #dde1ea;
    border-radius: 7px;
    cursor: pointer;
    user-select: none;
    transition: background 0.12s;
}
.lang-header:hover { background: #e2e8f0; }
.lang-icon { font-size: 1.2em; }
.lang-name {
    font-size: 0.92em;
    font-weight: 700;
    color: #1e293b;
    flex: 1;
}
.lang-count {
    font-size: 0.75em;
    color: #94a3b8;
    background: #e2e8f0;
    border-radius: 10px;
    padding: 0.1em 0.55em;
}
.lang-chevron {
    font-size: 0.75em;
    color: #94a3b8;
    transition: transform 0.18s;
}
.lang-section.open .lang-chevron { transform: rotate(90deg); }

/* ── Extension grid ── */
.ext-grid {
    display: none;
    grid-template-columns: repeat(auto-fill, minmax(260px, 1fr));
    gap: 0.75em;
    padding: 0.6em 0.2em 0;
}
.lang-section.open .ext-grid { display: grid; }

/* ── Extension card ── */
.ext-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 9px;
    padding: 0.85em 1em;
    display: flex;
    flex-direction: column;
    gap: 0.45em;
    transition: border-color 0.15s, box-shadow 0.15s;
}
.ext-card:hover {
    border-color: #6366f1;
    box-shadow: 0 2px 10px rgba(99,102,241,0.10);
}
.ext-name {
    font-size: 0.9em;
    font-weight: 700;
    color: #1e293b;
}
.ext-meta {
    font-size: 0.73em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #94a3b8;
}
.ext-actions { display: flex; gap: 0.4em; flex-wrap: wrap; margin-top: 0.15em; }

.build-btn {
    padding: 0.25em 0.7em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 4px;
    cursor: pointer;
    font-weight: 600;
    border: 1px solid #6366f1;
    background: #6366f1;
    color: #fff;
    transition: background 0.12s;
}
.build-btn:hover   { background: #4f46e5; border-color: #4f46e5; }
.build-btn:disabled { opacity: 0.5; cursor: not-allowed; }

.docs-btn {
    padding: 0.25em 0.6em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 4px;
    cursor: pointer;
    font-weight: 600;
    border: 1px solid #94a3b8;
    background: #f8fafc;
    color: #475569;
    transition: background 0.12s;
}
.docs-btn:hover { background: #e2e8f0; border-color: #64748b; }

/* ── Docs panel ── */
.docs-panel {
    font-size: 0.72em;
    font-family: inherit;
    background: #f8fafc;
    border: 1px solid #dde1ea;
    border-left: 3px solid #6366f1;
    border-radius: 5px;
    padding: 0.6em 0.8em;
    max-height: 200px;
    overflow-y: auto;
    white-space: pre-wrap;
    word-break: break-word;
    color: #334155;
    line-height: 1.5;
    animation: fadeIn 0.15s ease;
}

/* ── Build output ── */
.build-output {
    font-size: 0.72em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    background: #0f172a;
    color: #e2e8f0;
    border-radius: 5px;
    padding: 0.5em 0.7em;
    max-height: 120px;
    overflow-y: auto;
    white-space: pre-wrap;
    word-break: break-all;
    animation: fadeIn 0.15s ease;
}
.build-output.ok  { border-left: 3px solid #22c55e; }
.build-output.err { border-left: 3px solid #ef4444; }
@keyframes fadeIn { from { opacity: 0 } to { opacity: 1 } }

/* ── Artifact downloads ── */
.artifacts {
    display: flex;
    flex-wrap: wrap;
    gap: 0.4em;
    padding-top: 0.35em;
    animation: fadeIn 0.2s ease;
}
.artifact-link {
    display: inline-flex;
    align-items: center;
    gap: 0.3em;
    padding: 0.22em 0.6em;
    font-size: 0.75em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    border-radius: 4px;
    border: 1px solid #22c55e;
    color: #166534;
    background: #f0fdf4;
    text-decoration: none;
    transition: background 0.12s;
}
.artifact-link:hover { background: #dcfce7; }

/* ── Toolchain warning ── */
.toolchain-warn {
    font-size: 0.72em;
    color: #92400e;
    background: #fffbeb;
    border: 1px solid #fcd34d;
    border-radius: 4px;
    padding: 0.25em 0.5em;
    margin-top: 0.2em;
}

/* ── Installer panel ── */
.installer-panel {
    background: #f8fafc;
    border: 1.5px solid #dde1ea;
    border-radius: 9px;
    padding: 0.85em 1em 1em;
    margin-bottom: 1em;
    animation: fadeIn 0.15s ease;
}
.installer-panel h3 {
    margin: 0 0 0.7em;
    font-size: 0.88em;
    font-weight: 700;
    color: #1e293b;
    display: flex;
    align-items: center;
    gap: 0.4em;
}
.tools-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(180px, 1fr));
    gap: 0.55em;
    margin-bottom: 0.75em;
}
.tool-row {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.35em 0.6em;
    background: #fff;
    border: 1.5px solid #e2e8f0;
    border-radius: 6px;
    font-size: 0.8em;
    transition: border-color 0.12s;
}
.tool-row.installed { border-color: #22c55e; }
.tool-row.missing   { border-color: #f97316; }
.tool-status { font-size: 0.85em; margin-left: auto; }
.tool-row.installed .tool-status { color: #16a34a; }
.tool-row.missing   .tool-status { color: #ea580c; }
.install-btn {
    padding: 0.2em 0.55em;
    font-size: 0.75em;
    font-family: inherit;
    font-weight: 600;
    border-radius: 4px;
    cursor: pointer;
    border: 1px solid #6366f1;
    background: #6366f1;
    color: #fff;
    transition: background 0.12s;
    white-space: nowrap;
}
.install-btn:hover    { background: #4f46e5; border-color: #4f46e5; }
.install-btn:disabled { opacity: 0.45; cursor: not-allowed; }
.install-all-btn {
    padding: 0.28em 0.85em;
    font-size: 0.8em;
    font-family: inherit;
    font-weight: 600;
    border-radius: 5px;
    cursor: pointer;
    border: 1px solid #0ea5e9;
    background: #0ea5e9;
    color: #fff;
    transition: background 0.12s;
}
.install-all-btn:hover    { background: #0284c7; }
.install-all-btn:disabled { opacity: 0.45; cursor: not-allowed; }
.install-output {
    font-size: 0.7em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    background: #0f172a;
    color: #e2e8f0;
    border-radius: 5px;
    padding: 0.5em 0.7em;
    max-height: 160px;
    overflow-y: auto;
    white-space: pre-wrap;
    word-break: break-all;
    margin-top: 0.6em;
    display: none;
}
.install-output.active { display: block; }
.install-output.ok  { border-left: 3px solid #22c55e; }
.install-output.err { border-left: 3px solid #ef4444; }

/* ── Empty ── */
.empty {
    text-align: center;
    padding: 3em 1em;
    color: #94a3b8;
}
.empty-icon { font-size: 2.5rem; margin-bottom: 0.5em; }
`;

const LANG_ICONS = {
    c:          '⚙️',
    c3:         '🔩',
    d:          '🎯',
    emscripten: '🌐',
    go:         '🐹',
    java:       '☕',
    javascript: '🟨',
    postgres:   '🐘',
    python:     '🐍',
    r:          '📊',
    rust:       '🦀',
    swift:      '🕊️',
    v:          '🔷',
    zig:        '⚡',
};

const BUILD_TYPE_LABEL = {
    cmake:       'CMake',
    cargo:       'Cargo',
    go:          'go build',
    maven:       'Maven',
    npm:         'npm',
    python:      'pip install',
    emscripten:  'emcmake cmake',
    postgres:    'SQL package',
    swift:       'swift build',
    vlang:       'v build',
    c3:          'c3c compile',
    dlang:       'dub build',
    zig:         'zig build',
    r:           'R CMD build',
    unknown:     'unknown',
};

function _toolchainOk(binding) {
    if (binding.build_type === 'emscripten') return binding.has_emcc !== false;
    if (binding.build_type === 'go')         return binding.has_go_exec !== false;
    if (binding.build_type === 'maven')      return binding.has_mvn_exec !== false;
    if (binding.build_type === 'vlang')      return binding.has_v_exec !== false;
    if (binding.build_type === 'c3')         return binding.has_c3c_exec !== false;
    if (binding.build_type === 'dlang')      return binding.has_d_exec !== false;
    if (binding.build_type === 'zig')        return binding.has_zig_exec !== false;
    if (binding.build_type === 'r')          return binding.has_r_exec !== false;
    if (binding.build_type === 'swift')      return binding.has_swift_exec !== false;
    return true;
}

function _toolchainHint(binding) {
    if (binding.build_type === 'emscripten' && binding.has_emcc === false)
        return '⚠️ emcc not found — use Install Tools to set up emsdk';
    if (binding.build_type === 'go' && binding.has_go_exec === false)
        return '⚠️ go not found — use Install Tools or https://go.dev/dl/';
    if (binding.build_type === 'maven' && binding.has_mvn_exec === false)
        return '⚠️ Maven (mvn) not found — use Install Tools or https://maven.apache.org/download.cgi';
    if (binding.build_type === 'maven' && binding.has_java_exec === false)
        return '⚠️ JDK not found — Maven requires Java; use Install Tools to install JDK 21';
    if (binding.build_type === 'vlang' && binding.has_v_exec === false)
        return '⚠️ V compiler not found — use Install Tools or https://vlang.io';
    if (binding.build_type === 'c3' && binding.has_c3c_exec === false)
        return '⚠️ c3c not found — use Install Tools or https://c3-lang.org';
    if (binding.build_type === 'dlang' && binding.has_d_exec === false)
        return '⚠️ D toolchain not found — use Install Tools or https://dlang.org';
    if (binding.build_type === 'zig' && binding.has_zig_exec === false)
        return '⚠️ Zig compiler not found — use Install Tools or https://ziglang.org/download/';
    if (binding.build_type === 'r' && binding.has_r_exec === false)
        return '⚠️ R not found — use Install Tools or https://cran.r-project.org';
    if (binding.build_type === 'r' && binding.has_rtools === false)
        return '⚠️ Rtools not found — use Install Tools to set up gcc for R packages';
    if (binding.build_type === 'swift' && binding.has_swift_exec === false)
        return '⚠️ Swift toolchain not found — install from https://www.swift.org/download/';
    if (binding.build_type === 'postgres' && binding.has_psql_exec === false)
        return '⚠️ psql not found — install PostgreSQL client tools to enable live SQL validation';
    return null;
}

function langIcon(lang)  { return LANG_ICONS[lang.toLowerCase()] ?? '📦'; }
function langLabel(lang) { return lang; }

class ExtensionBuilder extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        this._shadow = shadow;

        // ── Toolbar ──
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';
        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'btn';
        refreshBtn.textContent = '↻ Refresh';
        const installBtn = document.createElement('button');
        installBtn.className = 'btn';
        installBtn.textContent = '🛠 Install Tools';
        this._statusEl = document.createElement('span');
        this._statusEl.className = 'status';
        toolbar.appendChild(refreshBtn);
        toolbar.appendChild(installBtn);
        toolbar.appendChild(this._statusEl);
        shadow.appendChild(toolbar);

        // ── Installer panel (hidden until toggled) ──
        this._installerPanel = this._makeInstallerPanel();
        shadow.appendChild(this._installerPanel);
        installBtn.addEventListener('click', () => {
            const visible = this._installerPanel.style.display !== 'none';
            this._installerPanel.style.display = visible ? 'none' : 'block';
            if (!visible) this._loadTools();
        });
        this._installerPanel.style.display = 'none';

        // ── Content area (flat card grid) ──
        this._content = document.createElement('div');
        this._content.style.cssText = 'display:grid;grid-template-columns:repeat(auto-fill,minmax(260px,1fr));gap:0.75em;padding-top:0.4em;';
        shadow.appendChild(this._content);

        refreshBtn.addEventListener('click', () => this._load());
        this._load();
    }

    // ── Installer panel ──────────────────────────────────────────────────────

    _makeInstallerPanel() {
        const panel = document.createElement('div');
        panel.className = 'installer-panel';
        panel.innerHTML = `<h3>🛠 Toolchain Installer</h3>`;

        this._toolsGrid = document.createElement('div');
        this._toolsGrid.className = 'tools-grid';
        this._toolsGrid.textContent = 'Loading…';
        panel.appendChild(this._toolsGrid);

        const footer = document.createElement('div');
        footer.style.display = 'flex';
        footer.style.gap = '0.5em';
        footer.style.alignItems = 'center';

        const installAll = document.createElement('button');
        installAll.className = 'install-all-btn';
        installAll.textContent = '⬇ Install All Missing';
        installAll.addEventListener('click', () => this._installTool('all', installAll));
        footer.appendChild(installAll);

        const recheck = document.createElement('button');
        recheck.className = 'btn';
        recheck.style.fontSize = '0.78em';
        recheck.textContent = '↻ Re-check';
        recheck.addEventListener('click', () => this._loadTools());
        footer.appendChild(recheck);

        panel.appendChild(footer);

        this._installOutput = document.createElement('pre');
        this._installOutput.className = 'install-output';
        panel.appendChild(this._installOutput);

        return panel;
    }

    async _loadTools() {
        this._toolsGrid.textContent = 'Checking…';
        try {
            const r = await fetch('/extensions/tools');
            const data = await r.json();
            this._toolsGrid.innerHTML = '';
            for (const t of data.tools) {
                const row = document.createElement('div');
                row.className = `tool-row ${t.installed ? 'installed' : 'missing'}`;
                row.dataset.tool = t.tool;

                const label = document.createElement('span');
                label.textContent = t.label;
                row.appendChild(label);

                const status = document.createElement('span');
                status.className = 'tool-status';
                status.textContent = t.installed ? '✓' : '✗';
                row.appendChild(status);

                if (!t.installed) {
                    const btn = document.createElement('button');
                    btn.className = 'install-btn';
                    btn.textContent = '⬇ Install';
                    btn.addEventListener('click', () => this._installTool(t.tool, btn));
                    row.appendChild(btn);
                }
                this._toolsGrid.appendChild(row);
            }
        } catch (e) {
            this._toolsGrid.textContent = `Error: ${e.message}`;
        }
    }

    async _installTool(tool, triggerBtn) {
        triggerBtn.disabled = true;

        const out = this._installOutput;
        out.textContent = '';
        out.className = 'install-output active';
        out.scrollTop = 0;

        try {
            const r = await fetch('/extensions/install', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ tool }),
            });
            if (!r.ok || !r.body) {
                out.textContent = `Request failed: ${r.status}`;
                out.classList.add('err');
                triggerBtn.disabled = false;
                return;
            }
            const reader = r.body.getReader();
            const dec = new TextDecoder();
            let exitCode = null;
            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                const chunk = dec.decode(value, { stream: true });
                const codeMatch = chunk.match(/__EXIT_CODE__:(\d+)/);
                if (codeMatch) exitCode = parseInt(codeMatch[1], 10);
                out.textContent += chunk.replace(/__EXIT_CODE__:\d+\n?/, '');
                out.scrollTop = out.scrollHeight;
            }
            const ok = exitCode === 0;
            out.classList.toggle('ok', ok);
            out.classList.toggle('err', !ok);
            // Refresh tool rows after install
            await this._loadTools();
            // Refresh binding cards so warning badges update
            await this._load();
        } catch (e) {
            out.textContent += `\nERROR: ${e.message}`;
            out.classList.add('err');
        } finally {
            triggerBtn.disabled = false;
        }
    }

    async _load() {
        this._statusEl.textContent = 'Loading…';
        this._content.innerHTML = '';

        let data;
        try {
            const r = await fetch('/extensions');
            data = await r.json();
        } catch (e) {
            this._content.innerHTML = `<div class="empty"><div class="empty-icon">❌</div><div>Failed to load extensions: ${e.message}</div></div>`;
            this._statusEl.textContent = '';
            return;
        }

        if (!data.bindings || !data.bindings.length) {
            this._content.innerHTML = `<div class="empty"><div class="empty-icon">🧩</div><div>No bindings found in _deliverables/libraries/bindings/.</div></div>`;
            this._statusEl.textContent = '';
            return;
        }

        for (const b of data.bindings) {
            this._content.appendChild(this._makeBindingCard(b));
        }
        this._statusEl.textContent = `${data.bindings.length} binding${data.bindings.length !== 1 ? 's' : ''}`;
    }

    _makeBindingCard(binding) {
        const card = document.createElement('div');
        card.className = 'ext-card';

        const name = document.createElement('div');
        name.className = 'ext-name';
        name.innerHTML = `${langIcon(binding.lang)} ${binding.lang}`;
        card.appendChild(name);

        const meta = document.createElement('div');
        meta.className = 'ext-meta';
        const targetLabel = binding.cmake_target
            ? `${BUILD_TYPE_LABEL[binding.build_type] ?? binding.build_type} → ${binding.cmake_target}`
            : (BUILD_TYPE_LABEL[binding.build_type] ?? binding.build_type);
        meta.textContent = `${binding.name}  ·  ${targetLabel}`;
        card.appendChild(meta);

        const actions = document.createElement('div');
        actions.className = 'ext-actions';

        const hint = _toolchainHint(binding);
        if (hint) {
            const w = document.createElement('div');
            w.className = 'toolchain-warn';
            w.textContent = hint;
            card.appendChild(w);
        }

        if (binding.build_type !== 'unknown') {
            const buildBtn = document.createElement('button');
            buildBtn.className = 'build-btn';
            buildBtn.textContent = '\ud83d\udd28 Build';
            if (!_toolchainOk(binding)) buildBtn.disabled = true;
            buildBtn.addEventListener('click', () => this._build(binding, buildBtn, card));
            actions.appendChild(buildBtn);
        }

        const docsBtn = document.createElement('button');
        docsBtn.className = 'docs-btn';
        docsBtn.textContent = '\ud83d\udcc4 Docs';
        docsBtn.addEventListener('click', () => this._toggleDocs(binding, docsBtn, card));
        actions.appendChild(docsBtn);

        card.appendChild(actions);
        return card;
    }

    async _build(binding, btn, card) {
        btn.disabled = true;
        btn.textContent = '⏳ Building…';

        // Remove any previous output
        const prev = card.querySelector('.build-output');
        if (prev) prev.remove();

        const out = document.createElement('pre');
        out.className = 'build-output';
        out.textContent = '';
        card.appendChild(out);

        let success = false;
        try {
            const r = await fetch('/extensions/build', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ binding: binding.name }),
            });

            const reader = r.body.getReader();
            const decoder = new TextDecoder();
            let buffer = '';

            while (true) {
                const { done, value } = await reader.read();
                if (done) break;
                buffer += decoder.decode(value, { stream: true });

                // Extract exit code sentinel if present
                const exitMatch = buffer.match(/__EXIT_CODE__:(\d+)/);
                if (exitMatch) {
                    success = parseInt(exitMatch[1], 10) === 0;
                    buffer = buffer.replace(/__EXIT_CODE__:\d+\n?/, '');
                }

                out.textContent = buffer;
                out.scrollTop = out.scrollHeight;
            }
        } catch (e) {
            out.textContent += `\nError: ${e.message}`;
            success = false;
        }

        out.className = 'build-output ' + (success ? 'ok' : 'err');
        btn.disabled = false;
        btn.textContent = '🔨 Build';

        if (success) {
            await this._showArtifacts(binding, card);
        }
    }

    async _showArtifacts(binding, card) {
        // Remove previous artifact row
        const prev = card.querySelector('.artifacts');
        if (prev) prev.remove();

        let data;
        try {
            const r = await fetch(`/extensions/artifacts?binding=${encodeURIComponent(binding.name)}`);
            data = await r.json();
        } catch { return; }

        if (!data.artifacts || !data.artifacts.length) return;

        const row = document.createElement('div');
        row.className = 'artifacts';
        for (const art of data.artifacts) {
            const a = document.createElement('a');
            a.className = 'artifact-link';
            a.href = art.url;
            a.download = art.name;
            a.title = `${(art.size / 1024).toFixed(1)} KB`;
            a.textContent = `\u2b07 ${art.name}`;
            row.appendChild(a);
        }
        card.appendChild(row);
    }

    async _toggleDocs(binding, btn, card) {
        const existing = card.querySelector('.docs-panel');
        if (existing) {
            existing.remove();
            return;
        }
        btn.textContent = '\u23f3 Loading…';
        const panel = document.createElement('pre');
        panel.className = 'docs-panel';
        try {
            const r = await fetch(`/extensions/docs?binding=${encodeURIComponent(binding.name)}`);
            if (r.ok) {
                const data = await r.json();
                panel.textContent = data.content || '(no README found in binding directory)';
            } else {
                panel.textContent = '(docs unavailable)';
            }
        } catch (e) {
            panel.textContent = `Error: ${e.message}`;
        }
        card.appendChild(panel);
        btn.textContent = '\ud83d\udcc4 Docs';
    }
}

customElements.define('extension-builder', ExtensionBuilder);
