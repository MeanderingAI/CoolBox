/**
 * <workspace-editor>
 * Full-repo file browser + editable textarea.
 * Left: lazy-loading directory tree (starts at repo root).
 * Right: text editor with Save / Discard / Ctrl+S.
 * Endpoints:
 *   GET  /workspace/tree?path=   → { path, entries: [{name, path, type, size, editable}] }
 *   GET  /workspace/read?path=   → { path, content, size }
 *   POST /workspace/write        → { path, content }
 */

// ── Language → highlight style mapping ──────────────────────────────────────
const EXT_LANG = {
    c:'c', cc:'cpp', cpp:'cpp', cxx:'cpp', h:'c', hpp:'cpp', hxx:'cpp', inl:'cpp',
    py:'python', pyi:'python',
    js:'js', mjs:'js', jsx:'js', cjs:'js',
    ts:'ts', tsx:'ts',
    json:'json', jsonc:'json', toml:'toml', yaml:'yaml', yml:'yaml',
    md:'markdown', rst:'markdown',
    html:'html', htm:'html', css:'css', scss:'css', svg:'xml', xml:'xml',
    sh:'bash', bash:'bash', zsh:'bash', ps1:'bash',
    go:'go', rs:'rust', cmake:'cmake', sql:'sql',
    txt:'', log:'', csv:'',
};

const EXT_ICON = {
    // code
    c:'🔵',cc:'🔵',cpp:'🔵',cxx:'🔵',h:'🔷',hpp:'🔷',hxx:'🔷',inl:'🔷',
    py:'🐍', pyi:'🐍',
    js:'🟨', mjs:'🟨', cjs:'🟨', jsx:'🟨',
    ts:'🔹', tsx:'🔹',
    go:'🩵', rs:'🦀',
    // config / data
    json:'📋', jsonc:'📋', toml:'⚙️', yaml:'⚙️', yml:'⚙️', xml:'📄', xsd:'📄',
    md:'📝', rst:'📝', txt:'📄', log:'📃', csv:'📊', sql:'🗄️',
    // web
    html:'🌐', htm:'🌐', css:'🎨', scss:'🎨', less:'🎨', svg:'🖼️',
    // shell
    sh:'🖥️', bash:'🖥️', ps1:'🖥️', bat:'🖥️',
    // build
    cmake:'🔧', makefile:'🔧', mk:'🔧', tf:'🏗️',
    // catch-all
    _dir: '📁', _dir_open: '📂', _unknown: '📄',
};

function fileIcon(name, isDir, open = false) {
    if (isDir) return open ? EXT_ICON._dir_open : EXT_ICON._dir;
    const ext = name.includes('.') ? name.split('.').pop().toLowerCase() : '';
    return EXT_ICON[ext] ?? EXT_ICON._unknown;
}

function fmtSize(bytes) {
    if (bytes < 1024)        return `${bytes} B`;
    if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
    return `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
}

// ── Styles ───────────────────────────────────────────────────────────────────
const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    font-family: 'Segoe UI', system-ui, sans-serif;
    flex: 1;
    min-height: 0;
    height: 100%;
    background: #1e1e2e;
    color: #cdd6f4;
}

/* ── Top toolbar ── */
.we-toolbar {
    display: flex;
    align-items: center;
    gap: 0.4em;
    padding: 0.45em 0.75em;
    background: #181825;
    border-bottom: 1px solid #313244;
    flex-shrink: 0;
    flex-wrap: wrap;
}
.we-crumb {
    font-size: 0.78em;
    color: #89b4fa;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    min-width: 80px;
}
.we-dirty { color: #f9e2af; }
.we-btn {
    padding: 0.28em 0.7em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 5px;
    border: 1px solid #45475a;
    background: #313244;
    color: #cdd6f4;
    cursor: pointer;
    transition: background 0.12s;
    white-space: nowrap;
}
.we-btn:hover:not(:disabled) { background: #45475a; }
.we-btn:disabled { opacity: 0.4; cursor: default; }
.we-btn.save  { background: #1e66f5; border-color: #1e66f5; color: #fff; }
.we-btn.save:hover:not(:disabled) { background: #1558d8; }
.we-btn.discard { border-color: #f38ba8; color: #f38ba8; }
.we-btn.discard:hover:not(:disabled) { background: #302030; }
.we-status {
    font-size: 0.72em;
    color: #6c7086;
    margin-left: 0.25em;
    white-space: nowrap;
}
.we-status.ok  { color: #a6e3a1; }
.we-status.err { color: #f38ba8; }

/* ── Body = sidebar + editor ── */
.we-body {
    display: flex;
    flex: 1;
    min-height: 0;
    overflow: hidden;
}

/* ── Sidebar ── */
.we-sidebar {
    width: 240px;
    min-width: 120px;
    max-width: 400px;
    display: flex;
    flex-direction: column;
    background: #1e1e2e;
    border-right: 1px solid #313244;
    flex-shrink: 0;
    overflow: hidden;
}
.we-sidebar-hdr {
    display: flex;
    align-items: center;
    padding: 0.45em 0.65em;
    background: #181825;
    border-bottom: 1px solid #313244;
    font-size: 0.7em;
    font-weight: 700;
    letter-spacing: 0.09em;
    text-transform: uppercase;
    color: #6c7086;
    gap: 0.4em;
    flex-shrink: 0;
}
.we-sidebar-hdr span { flex: 1; }
.we-sidebar-refresh {
    background: none;
    border: none;
    color: #6c7086;
    cursor: pointer;
    padding: 0.1em 0.3em;
    border-radius: 3px;
    font-size: 1em;
    transition: color 0.12s;
}
.we-sidebar-refresh:hover { color: #cdd6f4; }
.we-tree {
    flex: 1;
    overflow-y: auto;
    overflow-x: hidden;
    padding: 0.25em 0;
}

/* ── Tree nodes ── */
.tree-node {
    display: flex;
    align-items: center;
    gap: 0;
    padding: 0.22em 0;
    cursor: pointer;
    user-select: none;
    border-radius: 4px;
    margin: 0 0.2em;
    transition: background 0.08s;
    font-size: 0.82em;
}
.tree-node:hover { background: #313244; }
.tree-node.selected { background: #45475a; }
.tree-node.selected .tn-name { color: #89b4fa; }
.tn-indent { flex-shrink: 0; }
.tn-toggle {
    width: 14px;
    text-align: center;
    flex-shrink: 0;
    color: #6c7086;
    font-size: 0.75em;
    transition: transform 0.12s;
}
.tn-toggle.open { transform: rotate(90deg); }
.tn-icon { flex-shrink: 0; margin-right: 0.35em; font-size: 0.95em; }
.tn-name {
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    color: #cdd6f4;
}
.tn-size {
    color: #45475a;
    font-size: 0.72em;
    margin-right: 0.4em;
    flex-shrink: 0;
    font-family: 'Cascadia Code', 'Consolas', monospace;
}

/* ── Children container ── */
.tree-children { display: none; }
.tree-children.open { display: block; }

/* ── Drag handle ── */
.we-drag {
    width: 4px;
    cursor: col-resize;
    background: #313244;
    flex-shrink: 0;
    transition: background 0.12s;
}
.we-drag:hover, .we-drag.dragging { background: #89b4fa; }

/* ── Editor pane ── */
.we-editor-pane {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    background: #1e1e2e;
}
.we-editor-hdr {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.38em 0.8em;
    background: #181825;
    border-bottom: 1px solid #313244;
    font-size: 0.78em;
    color: #6c7086;
    flex-shrink: 0;
}
.we-file-tab {
    padding: 0.2em 0.75em;
    background: #1e1e2e;
    border: 1px solid #313244;
    border-bottom: 1px solid #1e1e2e;
    border-radius: 5px 5px 0 0;
    color: #cdd6f4;
    font-size: 0.88em;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    max-width: 240px;
}
.we-cursor-pos { margin-left: auto; font-family: 'Cascadia Code','Consolas',monospace; }

/* ── Welcome / empty ── */
.we-welcome {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    color: #45475a;
    font-size: 0.88em;
    text-align: center;
    gap: 0.5em;
}
.we-welcome-icon { font-size: 3rem; margin-bottom: 0.25em; }

/* ── Textarea editor ── */
.we-textarea-wrap {
    flex: 1;
    display: flex;
    flex-direction: column;
    overflow: hidden;
    position: relative;
}
.we-lines {
    display: flex;
    flex: 1;
    overflow: hidden;
}
.we-line-nums {
    padding: 0.7em 0 0.7em 0.5em;
    background: #181825;
    border-right: 1px solid #313244;
    color: #45475a;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.8em;
    line-height: 1.6;
    text-align: right;
    overflow: hidden;
    flex-shrink: 0;
    min-width: 2.5em;
    padding-right: 0.5em;
    user-select: none;
}
.we-textarea {
    flex: 1;
    background: #1e1e2e;
    color: #cdd6f4;
    border: none;
    outline: none;
    resize: none;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace;
    font-size: 0.82em;
    line-height: 1.6;
    padding: 0.7em 1em;
    tab-size: 4;
    overflow-y: scroll;
    overflow-x: auto;
    white-space: pre;
}
.we-textarea::selection { background: #3d59a1; }

/* ── Status bar ── */
.we-statusbar {
    display: flex;
    align-items: center;
    gap: 1em;
    padding: 0.2em 0.75em;
    background: #1e66f5;
    color: #e6e9f4;
    font-size: 0.7em;
    flex-shrink: 0;
}
.we-statusbar.no-file { background: #181825; color: #6c7086; }
.we-statusbar span { white-space: nowrap; }
.sb-right { margin-left: auto; display: flex; gap: 1em; }

/* ── Loading / error state ── */
.we-msg {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 0.85em;
    color: #6c7086;
}
.we-msg.err { color: #f38ba8; }
`;

// ── Component ─────────────────────────────────────────────────────────────────
class WorkspaceEditor extends HTMLElement {
    connectedCallback() {
        if (this._built) return;
        this._built = true;
        this._currentPath = null;      // repo-relative path of open file
        this._originalContent = null;  // last-saved/loaded content
        this._dirCache = new Map();    // path → entries[]

        const shadow = this.attachShadow({ mode: 'open' });
        this._shadow = shadow;

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        this._buildShell(shadow);
        this._loadTree('');
    }

    // ── Shell layout ─────────────────────────────────────────────────────────
    _buildShell(shadow) {
        // ── Top toolbar ──
        const tb = document.createElement('div');
        tb.className = 'we-toolbar';
        this._crumb   = document.createElement('span');
        this._crumb.className = 'we-crumb';
        this._crumb.textContent = 'No file open';

        this._saveBtn    = this._mkBtn('💾 Save', 'save we-btn', true);
        this._discardBtn = this._mkBtn('↩ Discard', 'discard we-btn', true);
        this._reloadBtn  = this._mkBtn('↻ Reload', 'we-btn', true);
        this._statusMsg  = document.createElement('span');
        this._statusMsg.className = 'we-status';

        tb.append(this._crumb, this._saveBtn, this._discardBtn, this._reloadBtn, this._statusMsg);
        shadow.appendChild(tb);

        // ── Body ──
        const body = document.createElement('div');
        body.className = 'we-body';
        shadow.appendChild(body);

        // Sidebar
        const sidebar = document.createElement('div');
        sidebar.className = 'we-sidebar';

        const sideHdr = document.createElement('div');
        sideHdr.className = 'we-sidebar-hdr';
        const sideTitle = document.createElement('span');
        sideTitle.textContent = 'Explorer';
        const sideRefresh = document.createElement('button');
        sideRefresh.className = 'we-sidebar-refresh';
        sideRefresh.title = 'Refresh tree';
        sideRefresh.textContent = '↻';
        sideHdr.append(sideTitle, sideRefresh);

        const tree = document.createElement('div');
        tree.className = 'we-tree';
        this._treeEl = tree;

        sidebar.append(sideHdr, tree);
        body.appendChild(sidebar);

        // Drag handle
        const drag = document.createElement('div');
        drag.className = 'we-drag';
        body.appendChild(drag);
        this._initDrag(drag, sidebar);

        // Editor pane
        const pane = document.createElement('div');
        pane.className = 'we-editor-pane';
        this._pane = pane;
        body.appendChild(pane);

        // Welcome screen
        this._showWelcome();

        // Status bar
        const sb = document.createElement('div');
        sb.className = 'we-statusbar no-file';
        this._sbLeft  = document.createElement('span');
        this._sbRight = document.createElement('span');
        this._sbRight.className = 'sb-right';
        sb.append(this._sbLeft, this._sbRight);
        this._statusBar = sb;
        shadow.appendChild(sb);

        // ── Events ──
        this._saveBtn.addEventListener('click',    () => this._save());
        this._discardBtn.addEventListener('click', () => this._discard());
        this._reloadBtn.addEventListener('click',  () => { if (this._currentPath) this._open(this._currentPath); });
        sideRefresh.addEventListener('click', () => { this._dirCache.clear(); this._loadTree(''); });
    }

    _mkBtn(label, cls, disabled = false) {
        const b = document.createElement('button');
        b.className = cls;
        b.textContent = label;
        b.disabled = disabled;
        return b;
    }

    // ── Welcome pane ─────────────────────────────────────────────────────────
    _showWelcome() {
        this._pane.innerHTML = '';
        const w = document.createElement('div');
        w.className = 'we-welcome';
        w.innerHTML = `<div class="we-welcome-icon">✏️</div>
            <div style="font-size:1.05em;font-weight:700;color:#6c7086">Workspace Editor</div>
            <div style="color:#45475a;margin-top:0.2em">Select a file from the explorer to open it.</div>
            <div style="color:#45475a;font-size:0.85em">Ctrl+S saves · Tab inserts spaces</div>`;
        this._pane.appendChild(w);
    }

    // ── Sidebar drag resize ───────────────────────────────────────────────────
    _initDrag(handle, sidebar) {
        let startX, startW;
        handle.addEventListener('mousedown', e => {
            startX = e.clientX;
            startW = sidebar.offsetWidth;
            handle.classList.add('dragging');
            const onMove = ev => {
                const w = Math.min(400, Math.max(120, startW + ev.clientX - startX));
                sidebar.style.width = w + 'px';
            };
            const onUp = () => {
                handle.classList.remove('dragging');
                window.removeEventListener('mousemove', onMove);
                window.removeEventListener('mouseup', onUp);
            };
            window.addEventListener('mousemove', onMove);
            window.addEventListener('mouseup', onUp);
            e.preventDefault();
        });
    }

    // ── Tree loading ──────────────────────────────────────────────────────────
    async _loadTree(path, container = null) {
        const target = container ?? this._treeEl;
        if (!container) {
            target.innerHTML = '<div style="padding:.5em 1em;color:#45475a;font-size:.8em">Loading…</div>';
        }

        try {
            const data = await fetch(`/workspace/tree?path=${encodeURIComponent(path)}`).then(r => r.json());
            if (data.error) throw new Error(data.error);
            this._dirCache.set(path, data.entries);
            target.innerHTML = '';
            this._renderEntries(data.entries, target, 0);
        } catch (err) {
            target.innerHTML = `<div style="padding:.5em 1em;color:#f38ba8;font-size:.8em">⚠ ${err.message}</div>`;
        }
    }

    _renderEntries(entries, container, depth) {
        for (const entry of entries) {
            const node = this._makeNode(entry, depth);
            container.appendChild(node);
        }
    }

    _makeNode(entry, depth) {
        const isDir = entry.type === 'dir';
        const wrapper = document.createElement('div');

        const row = document.createElement('div');
        row.className = 'tree-node';
        row.dataset.path = entry.path;
        row.dataset.type = entry.type;

        // Indent
        const indent = document.createElement('span');
        indent.className = 'tn-indent';
        indent.style.width = (depth * 16 + 6) + 'px';
        indent.style.display = 'inline-block';

        // Toggle arrow (dirs only)
        const toggle = document.createElement('span');
        toggle.className = 'tn-toggle';
        toggle.textContent = isDir ? '›' : '';

        // Icon
        const icon = document.createElement('span');
        icon.className = 'tn-icon';
        icon.textContent = fileIcon(entry.name, isDir);
        this._iconEl = icon; // updated on expand

        // Name
        const name = document.createElement('span');
        name.className = 'tn-name';
        name.textContent = entry.name;

        // Size (files only)
        const size = document.createElement('span');
        size.className = 'tn-size';
        if (!isDir && entry.size != null) size.textContent = fmtSize(entry.size);

        row.append(indent, toggle, icon, name, size);
        wrapper.appendChild(row);

        if (isDir) {
            const children = document.createElement('div');
            children.className = 'tree-children';
            wrapper.appendChild(children);
            let loaded = false;

            row.addEventListener('click', () => {
                const open = children.classList.toggle('open');
                toggle.classList.toggle('open', open);
                icon.textContent = fileIcon(entry.name, true, open);
                if (open && !loaded) {
                    loaded = true;
                    children.innerHTML = '<div style="padding:.3em 0 .3em 2.5em;color:#45475a;font-size:.75em">Loading…</div>';
                    this._loadTree(entry.path, children).then(() => {
                        this._renderEntries(this._dirCache.get(entry.path) || [], children, depth + 1);
                        children.innerHTML = '';
                        this._renderEntries(this._dirCache.get(entry.path) || [], children, depth + 1);
                    });
                }
            });
        } else {
            row.addEventListener('click', () => {
                if (!entry.editable) {
                    this._setStatus(`Binary file — cannot edit`, 'err');
                    return;
                }
                this._selectNode(row);
                this._open(entry.path);
            });
        }

        return wrapper;
    }

    _selectNode(row) {
        this._treeEl.querySelectorAll('.tree-node.selected').forEach(n => n.classList.remove('selected'));
        row.classList.add('selected');
    }

    // ── Open file ─────────────────────────────────────────────────────────────
    async _open(path) {
        if (this._dirty() && !confirm(`Discard unsaved changes to ${this._currentPath}?`)) return;
        this._setStatus('Loading…');
        try {
            const data = await fetch(`/workspace/read?path=${encodeURIComponent(path)}`).then(r => r.json());
            if (data.error) throw new Error(data.error);
            this._currentPath    = data.path;
            this._originalContent = data.content;
            this._buildEditor(data.path, data.content, data.size);
            this._setStatus('');
        } catch (err) {
            this._setStatus(`Error: ${err.message}`, 'err');
        }
    }

    // ── Build editor pane ─────────────────────────────────────────────────────
    _buildEditor(path, content, size) {
        this._pane.innerHTML = '';

        // Header with file tab
        const hdr = document.createElement('div');
        hdr.className = 'we-editor-hdr';
        const tab = document.createElement('span');
        tab.className = 'we-file-tab';
        tab.title = path;
        tab.textContent = path.split('/').pop();
        this._fileTab = tab;
        this._cursorPos = document.createElement('span');
        this._cursorPos.className = 'we-cursor-pos';
        this._cursorPos.textContent = 'Ln 1, Col 1';
        hdr.append(tab, this._cursorPos);
        this._pane.appendChild(hdr);

        // Lines + textarea
        const wrap = document.createElement('div');
        wrap.className = 'we-textarea-wrap';
        const lines = document.createElement('div');
        lines.className = 'we-lines';

        this._lineNums = document.createElement('div');
        this._lineNums.className = 'we-line-nums';

        const ta = document.createElement('textarea');
        ta.className = 'we-textarea';
        ta.value = content;
        ta.spellcheck = false;
        ta.autocomplete = 'off';
        ta.autocorrect = 'off';
        ta.autocapitalize = 'off';
        this._ta = ta;

        lines.append(this._lineNums, ta);
        wrap.appendChild(lines);
        this._pane.appendChild(wrap);

        this._updateLineNums(content);
        this._syncEditorButtons();

        this._crumb.textContent = path;
        this._crumb.classList.remove('we-dirty');

        this._statusBar.className = 'we-statusbar';
        this._sbLeft.textContent = path.split('/').pop();
        const ext = path.includes('.') ? path.split('.').pop().toLowerCase() : '';
        this._sbRight.textContent = `${ext ? ext.toUpperCase() : 'TEXT'} · ${fmtSize(size)} · UTF-8`;

        // Sync line numbers scroll
        ta.addEventListener('scroll', () => { this._lineNums.scrollTop = ta.scrollTop; });

        ta.addEventListener('input', () => {
            this._updateLineNums(ta.value);
            this._markDirty();
        });

        ta.addEventListener('keydown', (e) => {
            // Tab → insert spaces
            if (e.key === 'Tab') {
                e.preventDefault();
                const start = ta.selectionStart, end = ta.selectionEnd;
                const spaces = '    ';
                ta.value = ta.value.slice(0, start) + spaces + ta.value.slice(end);
                ta.selectionStart = ta.selectionEnd = start + spaces.length;
                this._markDirty();
                this._updateLineNums(ta.value);
            }
            // Ctrl+S / Cmd+S → save
            if ((e.ctrlKey || e.metaKey) && e.key === 's') {
                e.preventDefault();
                this._save();
            }
        });

        ta.addEventListener('keyup',   () => this._updateCursor());
        ta.addEventListener('click',   () => this._updateCursor());
        ta.addEventListener('mouseup', () => this._updateCursor());
    }

    _updateLineNums(text) {
        const count = (text.match(/\n/g) || []).length + 1;
        this._lineNums.textContent = Array.from({ length: count }, (_, i) => i + 1).join('\n');
    }

    _updateCursor() {
        if (!this._ta || !this._cursorPos) return;
        const val = this._ta.value;
        const pos = this._ta.selectionStart;
        const before = val.slice(0, pos);
        const lines = before.split('\n');
        const ln = lines.length;
        const col = lines[lines.length - 1].length + 1;
        this._cursorPos.textContent = `Ln ${ln}, Col ${col}`;
    }

    // ── Dirty state ───────────────────────────────────────────────────────────
    _dirty() {
        return this._ta && this._ta.value !== this._originalContent;
    }

    _markDirty() {
        this._crumb.classList.add('we-dirty');
        const name = this._currentPath?.split('/').pop() ?? '';
        if (this._fileTab) this._fileTab.textContent = `● ${name}`;
        this._syncEditorButtons();
    }

    _syncEditorButtons() {
        const hasFile = !!this._currentPath;
        const dirty   = this._dirty();
        this._saveBtn.disabled    = !hasFile || !dirty;
        this._discardBtn.disabled = !hasFile || !dirty;
        this._reloadBtn.disabled  = !hasFile;
    }

    // ── Save ──────────────────────────────────────────────────────────────────
    async _save() {
        if (!this._currentPath || !this._ta) return;
        this._saveBtn.disabled = true;
        this._setStatus('Saving…');
        try {
            const res = await fetch('/workspace/write', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ path: this._currentPath, content: this._ta.value }),
            }).then(r => r.json());
            if (!res.success) throw new Error(res.error || 'Unknown error');
            this._originalContent = this._ta.value;
            this._crumb.classList.remove('we-dirty');
            const name = this._currentPath.split('/').pop();
            if (this._fileTab) this._fileTab.textContent = name;
            this._setStatus(`✓ Saved (${fmtSize(res.bytes)})`, 'ok');
        } catch (err) {
            this._setStatus(`Save failed: ${err.message}`, 'err');
        }
        this._syncEditorButtons();
    }

    // ── Discard ───────────────────────────────────────────────────────────────
    _discard() {
        if (!this._ta || !this._originalContent) return;
        this._ta.value = this._originalContent;
        this._updateLineNums(this._originalContent);
        this._crumb.classList.remove('we-dirty');
        const name = this._currentPath?.split('/').pop() ?? '';
        if (this._fileTab) this._fileTab.textContent = name;
        this._setStatus('Changes discarded');
        this._syncEditorButtons();
    }

    // ── Status ────────────────────────────────────────────────────────────────
    _setStatus(msg, type = '') {
        this._statusMsg.textContent = msg;
        this._statusMsg.className = `we-status ${type}`;
        if (type === 'ok') setTimeout(() => { if (this._statusMsg.textContent === msg) this._statusMsg.textContent = ''; }, 2500);
    }
}

customElements.define('workspace-editor', WorkspaceEditor);
