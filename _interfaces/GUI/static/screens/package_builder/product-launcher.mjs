/**
 * <product-launcher>
 * Lists all products from _Product/, each with a Build button per cmake exe target
 * and a Launch button that starts the built exe as a detached process.
 * Endpoint: GET /products, POST /products/build, POST /products/launch
 */

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Grid of product cards ───────────────────────────── */
.product-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(280px, 1fr));
    gap: 1em;
    padding: 0.5em 0;
}

/* ── Product card ────────────────────────────────────── */
.product-card {
    background: #fff;
    border: 1.5px solid #dde1ea;
    border-radius: 10px;
    padding: 1em 1.1em 0.8em;
    display: flex;
    flex-direction: column;
    gap: 0.6em;
    transition: box-shadow 0.15s, border-color 0.15s;
}
.product-card:hover {
    border-color: #0e639c;
    box-shadow: 0 3px 12px rgba(14,99,156,0.12);
}

/* ── Header row ──────────────────────────────────────── */
.pc-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
}
.pc-icon { font-size: 1.4em; }
.pc-name {
    font-size: 1em;
    font-weight: 700;
    color: #1f2937;
    flex: 1;
}
.pc-folder {
    font-size: 0.7em;
    color: #9ca3af;
    font-family: 'Cascadia Code', 'Consolas', monospace;
}

/* ── Exe targets ─────────────────────────────────────── */
.pc-targets-label {
    font-size: 0.7em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6b7280;
    margin-bottom: 0.1em;
}
.pc-target-row {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.25em 0;
    border-bottom: 1px solid #f3f4f6;
}
.pc-target-row:last-child { border-bottom: none; }

.pc-target-name {
    font-size: 0.8em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #374151;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* ── Action buttons ──────────────────────────────────── */
.pc-btn {
    font-size: 0.75em;
    padding: 3px 10px;
    border-radius: 5px;
    border: 1px solid;
    cursor: pointer;
    font-family: inherit;
    transition: background 0.12s, color 0.12s;
    white-space: nowrap;
    flex-shrink: 0;
    text-decoration: none;
    display: inline-block;
}
.pc-btn:disabled { opacity: 0.5; cursor: default; }

.pc-btn.build {
    background: #eff6ff;
    border-color: #93c5fd;
    color: #1d4ed8;
}
.pc-btn.build:hover:not(:disabled) { background: #dbeafe; }

.pc-btn.launch {
    background: #f0fdf4;
    border-color: #86efac;
    color: #166534;
}
.pc-btn.launch:hover:not(:disabled) { background: #dcfce7; }

.pc-btn.info {
    background: #fafafa;
    border-color: #d1d5db;
    color: #6b7280;
}
.pc-btn.info:hover { background: #f3f4f6; color: #374151; }

/* ── Status + console ────────────────────────────────── */
.pc-status {
    font-size: 0.75em;
    min-height: 1.2em;
}
.pc-status.ok     { color: #16a34a; }
.pc-status.fail   { color: #dc2626; }
.pc-status.busy   { color: #6b7280; }

.pc-console {
    display: none;
    font-size: 0.72em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    background: #1e1e1e;
    color: #d4d4d4;
    padding: 0.5em 0.7em;
    border-radius: 5px;
    max-height: 200px;
    overflow: auto;
    white-space: pre-wrap;
    word-break: break-word;
}
.pc-console.visible { display: block; }

/* ── Empty / error states ────────────────────────────── */
.empty-state {
    color: #9ca3af;
    font-size: 0.9em;
    padding: 2em 0;
    text-align: center;
}
`;

const ICONS = {
    MStudio: '🎛️',
    bower_shell: '🐚',
    file_browser: '📂',
    body_generator: '🧬',
    default: '📦',
};

class ProductLauncher extends HTMLElement {
    async connectedCallback() {
        if (this.dataset.mounted) return;
        this.dataset.mounted = '1';

        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        this._root = shadow;
        await this._load();
    }

    async _load() {
        const placeholder = document.createElement('div');
        placeholder.className = 'empty-state';
        placeholder.textContent = 'Loading products…';
        this._root.appendChild(placeholder);

        try {
            const r = await fetch('/products');
            const data = await r.json();
            placeholder.remove();
            this._render(data.products || []);
        } catch (e) {
            placeholder.textContent = `Error loading products: ${e}`;
        }
    }

    _render(products) {
        if (!products.length) {
            const el = document.createElement('div');
            el.className = 'empty-state';
            el.textContent = 'No products found in _Product/.';
            this._root.appendChild(el);
            return;
        }

        const grid = document.createElement('div');
        grid.className = 'product-grid';

        products.forEach(product => {
            const card = this._makeCard(product);
            grid.appendChild(card);
        });

        this._root.appendChild(grid);
    }

    _makeCard(product) {
        const card = document.createElement('div');
        card.className = 'product-card';

        // ── Header ──
        const header = document.createElement('div');
        header.className = 'pc-header';

        const icon = document.createElement('span');
        icon.className = 'pc-icon';
        icon.textContent = ICONS[product.name] || ICONS.default;

        const name = document.createElement('span');
        name.className = 'pc-name';
        name.textContent = product.name;

        const folder = document.createElement('span');
        folder.className = 'pc-folder';
        folder.textContent = `_Product/${product.folder}`;

        header.appendChild(icon);
        header.appendChild(name);
        header.appendChild(folder);

        if (product.has_page) {
            const infoLink = document.createElement('a');
            infoLink.className = 'pc-btn info';
            infoLink.textContent = 'ℹ Info';
            infoLink.href = `/product-page/${encodeURIComponent(product.folder)}`;
            infoLink.target = '_blank';
            infoLink.rel = 'noopener noreferrer';
            infoLink.title = `Open ${product.name} product page`;
            header.appendChild(infoLink);
        }

        card.appendChild(header);

        // ── Status + console (shared across all targets on this card) ──
        const status = document.createElement('div');
        status.className = 'pc-status';

        const consoleEl = document.createElement('pre');
        consoleEl.className = 'pc-console';

        const setStatus = (msg, state) => {
            status.textContent = msg;
            status.className = 'pc-status ' + (state || '');
        };
        const showConsole = (text) => {
            consoleEl.textContent = text;
            consoleEl.classList.toggle('visible', !!text);
        };

        // ── Exe targets ──
        const exes = product.executables || [];
        if (exes.length) {
            const label = document.createElement('div');
            label.className = 'pc-targets-label';
            label.textContent = exes.length > 1 ? 'Executables' : 'Executable';
            card.appendChild(label);

            exes.forEach(exe => {
                const row = document.createElement('div');
                row.className = 'pc-target-row';

                const exeName = document.createElement('span');
                exeName.className = 'pc-target-name';
                exeName.textContent = exe;
                exeName.title = exe;

                const buildBtn = document.createElement('button');
                buildBtn.className = 'pc-btn build';
                buildBtn.textContent = '▶ Build';
                buildBtn.title = `cmake --build target: ${exe}`;

                const launchBtn = document.createElement('button');
                launchBtn.className = 'pc-btn launch';
                launchBtn.textContent = '🚀 Launch';
                launchBtn.title = `Launch ${exe}`;

                buildBtn.addEventListener('click', async () => {
                    buildBtn.disabled = true;
                    launchBtn.disabled = true;
                    setStatus(`Building ${exe}…`, 'busy');
                    showConsole('');
                    try {
                        const res = await fetch('/products/build', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ name: product.name, target: exe }),
                        });
                        const j = await res.json();
                        setStatus(j.success ? `✓ ${exe} built` : `✗ Build failed`, j.success ? 'ok' : 'fail');
                        showConsole(j.output || '');
                    } catch (e) {
                        setStatus(`Error: ${e}`, 'fail');
                    } finally {
                        buildBtn.disabled = false;
                        launchBtn.disabled = false;
                    }
                });

                launchBtn.addEventListener('click', async () => {
                    launchBtn.disabled = true;
                    setStatus(`Launching ${exe}…`, 'busy');
                    showConsole('');
                    try {
                        const res = await fetch('/products/launch', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ name: product.name, exe }),
                        });
                        const j = await res.json();
                        setStatus(j.success ? `✓ ${exe} launched` : `✗ ${j.output}`, j.success ? 'ok' : 'fail');
                        if (!j.success) showConsole(j.output || '');
                    } catch (e) {
                        setStatus(`Error: ${e}`, 'fail');
                    } finally {
                        launchBtn.disabled = false;
                    }
                });

                row.appendChild(exeName);
                row.appendChild(buildBtn);
                row.appendChild(launchBtn);
                card.appendChild(row);
            });
        } else {
            const none = document.createElement('div');
            none.style.cssText = 'font-size:0.75em;color:#9ca3af;font-style:italic;';
            none.textContent = 'No add_executable targets found in CMakeLists.txt';
            card.appendChild(none);
        }

        card.appendChild(status);
        card.appendChild(consoleEl);
        return card;
    }
}

customElements.define('product-launcher', ProductLauncher);
