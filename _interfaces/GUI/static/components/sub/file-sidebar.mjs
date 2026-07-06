/**
 * <file-sidebar>
 * A collapsible tree of library → header files.
 * Call setLibs(libs) with the array from /library/info to populate.
 * Dispatches a 'file-select' CustomEvent (bubbles, composed) with
 *   detail: { path: string, name: string }
 * when a header file is clicked.
 */

const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    height: 100%;
    min-height: 0;
    background: #252526;
    border-right: 1px solid #3e3e42;
    overflow: hidden;
    font-family: 'Segoe UI', system-ui, sans-serif;
    user-select: none;
}

/* ── Section header ──────────────────────────────────── */
.fs-title {
    font-size: 0.7em;
    font-weight: 700;
    letter-spacing: 0.08em;
    text-transform: uppercase;
    color: #bbb;
    padding: 0.6em 0.8em 0.4em;
    flex-shrink: 0;
    border-bottom: 1px solid #3e3e42;
}

/* ── Scrollable tree ─────────────────────────────────── */
.fs-tree {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
}
.fs-empty {
    color: #666;
    font-size: 0.8em;
    padding: 1em 0.8em;
    font-style: italic;
}

/* ── Library section ─────────────────────────────────── */
.lib-header {
    display: flex;
    align-items: center;
    gap: 0.4em;
    padding: 0.4em 0.6em;
    cursor: pointer;
    border-bottom: 1px solid #2d2d30;
    transition: background 0.1s;
}
.lib-header:hover { background: #2a2d2e; }

.lib-toggle {
    color: #9ca3af;
    font-size: 0.7em;
    width: 10px;
    text-align: center;
    flex-shrink: 0;
    transition: transform 0.12s;
}
.lib-toggle.open { transform: rotate(90deg); }

.lib-name {
    font-size: 0.8em;
    font-weight: 600;
    color: #ccc;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.lib-target {
    font-size: 0.68em;
    color: #666;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    background: #1e1e1e;
    padding: 1px 4px;
    border-radius: 3px;
    flex-shrink: 0;
    max-width: 100px;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

/* ── File list ───────────────────────────────────────── */
.file-list { display: none; }
.file-list.open { display: block; }

.file-item {
    display: flex;
    align-items: center;
    gap: 0.4em;
    padding: 0.28em 0.6em 0.28em 1.6em;
    cursor: pointer;
    font-size: 0.78em;
    color: #9cdcfe;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
    transition: background 0.1s;
    border-left: 2px solid transparent;
}
.file-item:hover { background: #2a2d2e; }
.file-item.selected {
    background: #094771;
    border-left-color: #0e639c;
    color: #fff;
}
.file-icon {
    color: #75beff;
    font-size: 0.9em;
    flex-shrink: 0;
}

/* ── Empty-headers note ──────────────────────────────── */
.no-headers {
    padding: 0.25em 0.6em 0.25em 1.6em;
    font-size: 0.72em;
    color: #555;
    font-style: italic;
}
`;

class FileSidebar extends HTMLElement {
    connectedCallback() {
        if (!this.shadowRoot) this._build();
    }

    _build() {
        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        const title = document.createElement('div');
        title.className = 'fs-title';
        title.textContent = 'Libraries';
        shadow.appendChild(title);

        this._tree = document.createElement('div');
        this._tree.className = 'fs-tree';
        shadow.appendChild(this._tree);

        this._selectedItem = null;
    }

    /** Populate the sidebar with library data from /library/info */
    setLibs(libs) {
        if (!this.shadowRoot) this._build();
        const tree = this._tree;
        tree.innerHTML = '';
        this._selectedItem = null;

        if (!libs || !libs.length) {
            const empty = document.createElement('div');
            empty.className = 'fs-empty';
            empty.textContent = 'No libraries found.';
            tree.appendChild(empty);
            return;
        }

        libs.forEach(lib => {
            const section = document.createElement('div');

            // ── Library header row ──
            const header = document.createElement('div');
            header.className = 'lib-header';

            const toggle = document.createElement('span');
            toggle.className = 'lib-toggle open';
            toggle.textContent = '▶';

            const nameEl = document.createElement('span');
            nameEl.className = 'lib-name';
            nameEl.textContent = lib.name;
            nameEl.title = lib.name;

            header.appendChild(toggle);
            header.appendChild(nameEl);

            if (lib.cmake_target) {
                const target = document.createElement('span');
                target.className = 'lib-target';
                target.textContent = lib.cmake_target;
                target.title = `cmake target: ${lib.cmake_target}`;
                header.appendChild(target);
            }

            // ── File list ──
            const fileList = document.createElement('div');
            fileList.className = 'file-list open';

            if (!lib.headers || !lib.headers.length) {
                const note = document.createElement('div');
                note.className = 'no-headers';
                note.textContent = 'No public headers';
                fileList.appendChild(note);
            } else {
                lib.headers.forEach(hdr => {
                    const item = document.createElement('div');
                    item.className = 'file-item';
                    item.title = hdr.path;

                    const icon = document.createElement('span');
                    icon.className = 'file-icon';
                    icon.textContent = '📄';

                    const label = document.createElement('span');
                    label.textContent = hdr.name;

                    item.appendChild(icon);
                    item.appendChild(label);

                    item.addEventListener('click', () => {
                        // Deselect previous
                        this._selectedItem?.classList.remove('selected');
                        item.classList.add('selected');
                        this._selectedItem = item;

                        this.dispatchEvent(new CustomEvent('file-select', {
                            detail: { path: hdr.path, name: hdr.name },
                            bubbles: true,
                            composed: true,
                        }));
                    });

                    fileList.appendChild(item);
                });
            }

            // Toggle collapse
            header.addEventListener('click', () => {
                const open = fileList.classList.toggle('open');
                toggle.classList.toggle('open', open);
            });

            section.appendChild(header);
            section.appendChild(fileList);
            tree.appendChild(section);
        });
    }
}

customElements.define('file-sidebar', FileSidebar);
