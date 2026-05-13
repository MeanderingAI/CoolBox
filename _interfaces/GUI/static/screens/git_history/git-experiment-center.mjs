// Custom element <git-experiment-center>
// Experiment Centre — per-session scratch workspace backed by a UUID folder
// inside _internal_workspace/temporary_workspaces/.
import '../workspace-editor.mjs';

const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    height: 100%;
    font-family: 'Segoe UI', system-ui, sans-serif;
    background: #1e1e2e;
    color: #cdd6f4;
}

/* ── Banner ── */
.banner {
    display: flex;
    align-items: center;
    gap: 0.7em;
    padding: 0.45em 0.85em;
    background: #181825;
    border-bottom: 1px solid #313244;
    flex-shrink: 0;
    flex-wrap: wrap;
}

.banner-label {
    font-size: 0.75em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6c7086;
}

.uuid-badge {
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.75em;
    background: #313244;
    border: 1px solid #45475a;
    border-radius: 5px;
    padding: 0.2em 0.6em;
    color: #89b4fa;
    user-select: all;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}

.new-btn {
    padding: 0.28em 0.75em;
    font-size: 0.78em;
    font-family: inherit;
    border-radius: 5px;
    border: 1px solid #45475a;
    background: #313244;
    color: #cdd6f4;
    cursor: pointer;
    transition: background 0.12s;
    white-space: nowrap;
    flex-shrink: 0;
}
.new-btn:hover { background: #45475a; }

/* ── Editor pane ── */
.editor-wrap {
    flex: 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

workspace-editor {
    flex: 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
}

/* ── Placeholder ── */
.placeholder {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    gap: 0.6em;
    color: #6c7086;
    font-size: 0.9em;
}
.placeholder .spinner {
    font-size: 1.8em;
    animation: spin 1.2s linear infinite;
}
@keyframes spin { to { transform: rotate(360deg); } }
`;

function _esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

class GitExperimentCenter extends HTMLElement {
    connectedCallback() {
        if (this._built) return;
        this._built = true;

        const shadow = this.attachShadow({ mode: 'open' });
        this._shadow = shadow;

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Banner
        const banner = document.createElement('div');
        banner.className = 'banner';

        const label = document.createElement('span');
        label.className = 'banner-label';
        label.textContent = '🧪 Experiment Centre';

        this._uuidBadge = document.createElement('span');
        this._uuidBadge.className = 'uuid-badge';
        this._uuidBadge.title = 'Workspace path (click to select all)';
        this._uuidBadge.textContent = 'Initialising…';

        const newBtn = document.createElement('button');
        newBtn.className = 'new-btn';
        newBtn.textContent = '+ New Workspace';
        newBtn.title = 'Create a fresh UUID workspace folder';
        newBtn.addEventListener('click', () => this._newWorkspace());

        banner.appendChild(label);
        banner.appendChild(this._uuidBadge);
        banner.appendChild(newBtn);
        shadow.appendChild(banner);

        // Editor wrap
        this._editorWrap = document.createElement('div');
        this._editorWrap.className = 'editor-wrap';
        this._editorWrap.innerHTML = `<div class="placeholder"><span class="spinner">⏳</span><span>Creating workspace…</span></div>`;
        shadow.appendChild(this._editorWrap);

        this._newWorkspace();
    }

    async _newWorkspace() {
        this._uuidBadge.textContent = 'Creating…';
        this._editorWrap.innerHTML = `<div class="placeholder"><span class="spinner">⏳</span><span>Creating workspace…</span></div>`;
        try {
            const res = await fetch('/experiment/new-workspace', { method: 'POST' });
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (!data.success) throw new Error(data.error || 'unknown error');
            this._uuidBadge.textContent = data.path;
            this._mountEditor(data.path);
        } catch (err) {
            this._uuidBadge.textContent = `Error: ${err.message}`;
            this._editorWrap.innerHTML = `<div class="placeholder"><span style="color:#f38ba8">⚠ ${_esc(err.message)}</span></div>`;
        }
    }

    _mountEditor(path) {
        this._editorWrap.innerHTML = '';
        const editor = document.createElement('workspace-editor');
        editor.setAttribute('root', path);
        this._editorWrap.appendChild(editor);
    }
}

customElements.define('git-experiment-center', GitExperimentCenter);

