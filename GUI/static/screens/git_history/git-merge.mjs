// Custom element <git-merge>
// Merge a selected branch into the current branch with a chosen strategy.

const STYLE = `
:host { display: block; font-family: inherit; }

.gm-wrap { padding: 1em; display: flex; flex-direction: column; gap: 1.2em; }

/* ── Card ───────────────────────────────────────────────── */
.gm-card {
    background: #f8faff;
    border: 1px solid #dde1ea;
    border-radius: 8px;
    overflow: hidden;
}

.gm-card-header {
    padding: 0.5em 0.9em;
    background: #f0f2f8;
    border-bottom: 1px solid #dde1ea;
    font-size: 0.75em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6b7280;
}

.gm-card-body { padding: 0.8em 1em; display: flex; flex-direction: column; gap: 0.7em; }

/* ── Current branch badge ───────────────────────────────── */
.gm-current-row {
    display: flex;
    align-items: center;
    gap: 0.6em;
    flex-wrap: wrap;
    font-size: 0.84em;
    color: #374151;
}

.gm-badge {
    display: inline-block;
    padding: 0.25em 0.7em;
    background: #dbeafe;
    border: 1px solid #93c5fd;
    border-radius: 4px;
    font-size: 0.85em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #1d4ed8;
    font-weight: 700;
}

.gm-badge.target {
    background: #d1fae5;
    border-color: #6ee7b7;
    color: #065f46;
}

/* ── Form row ───────────────────────────────────────────── */
.gm-row {
    display: flex;
    gap: 0.6em;
    align-items: center;
    flex-wrap: wrap;
}

.gm-label {
    font-size: 0.78em;
    font-weight: 700;
    letter-spacing: 0.05em;
    text-transform: uppercase;
    color: #6b7280;
    white-space: nowrap;
}

.gm-select {
    flex: 1;
    min-width: 160px;
    padding: 0.3em 0.6em;
    font-size: 0.85em;
    font-family: inherit;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    background: #fff;
    color: #1f2937;
}
.gm-select:focus { outline: 2px solid #0e639c; outline-offset: 1px; }

/* ── Strategy pills ─────────────────────────────────────── */
.gm-strategy-group {
    display: flex;
    gap: 0.4em;
    flex-wrap: wrap;
}

.gm-strategy-btn {
    padding: 0.28em 0.85em;
    font-size: 0.8em;
    font-family: inherit;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    cursor: pointer;
    color: #555;
    transition: background 0.1s, color 0.1s, border-color 0.1s;
}
.gm-strategy-btn:hover { background: #e8eaf0; }
.gm-strategy-btn[aria-pressed="true"] {
    background: #0e639c;
    border-color: #0e639c;
    color: #fff;
    font-weight: 600;
}

/* ── Merge button ───────────────────────────────────────── */
.gm-btn {
    padding: 0.35em 1.1em;
    font-size: 0.84em;
    font-family: inherit;
    background: #0e639c;
    color: #fff;
    border: none;
    border-radius: 5px;
    cursor: pointer;
    transition: background 0.12s;
    white-space: nowrap;
}
.gm-btn:hover  { background: #1177bb; }
.gm-btn:disabled { background: #9ca3af; cursor: default; }

/* ── Output console ─────────────────────────────────────── */
.gm-console {
    background: #1e1e2e;
    color: #d4d4d4;
    border-radius: 5px;
    padding: 0.6em 0.8em;
    font-size: 0.8em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    white-space: pre-wrap;
    word-break: break-all;
    max-height: 220px;
    overflow-y: auto;
    display: none;
}
.gm-console.visible { display: block; }
.gm-console .ok  { color: #4ec9b0; }
.gm-console .err { color: #f48771; }

/* ── Status banner ──────────────────────────────────────── */
.gm-status {
    padding: 0.4em 0.8em;
    border-radius: 5px;
    font-size: 0.82em;
    display: none;
}
.gm-status.ok  { background: #d1fae5; border: 1px solid #6ee7b7; color: #065f46; display: block; }
.gm-status.err { background: #fef2f2; border: 1px solid #fca5a5; color: #dc2626; display: block; }

/* ── Strategy info box ──────────────────────────────────── */
.gm-info {
    font-size: 0.78em;
    color: #6b7280;
    background: #f3f4f6;
    border-radius: 5px;
    padding: 0.4em 0.7em;
    line-height: 1.5;
}
`;

const STRATEGY_INFO = {
    'no-ff': 'Creates a merge commit even if a fast-forward is possible. Preserves branch history.',
    'ff':    'Fast-forward only — advances HEAD if possible. Fails if the branch has diverged.',
    'squash':'Squashes all source commits into a single staged change. You must commit manually after.',
};

class GitMerge extends HTMLElement {
    _strategy = 'no-ff';

    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        shadow.innerHTML = `<style>${STYLE}</style>`;

        const wrap = document.createElement('div');
        wrap.className = 'gm-wrap';
        shadow.appendChild(wrap);

        // ── Merge card ───────────────────────────────────────
        const card = document.createElement('div');
        card.className = 'gm-card';
        card.innerHTML = `<div class="gm-card-header">🔀 Merge Branch</div>`;

        const body = document.createElement('div');
        body.className = 'gm-card-body';

        // Current branch display
        const currentRow = document.createElement('div');
        currentRow.className = 'gm-current-row';
        this._currentBadge = document.createElement('span');
        this._currentBadge.className = 'gm-badge target';
        this._currentBadge.textContent = '…';
        currentRow.append('Merging into:', this._currentBadge);
        body.appendChild(currentRow);

        // Source branch selector
        const sourceRow = document.createElement('div');
        sourceRow.className = 'gm-row';
        const sourceLabel = document.createElement('span');
        sourceLabel.className = 'gm-label';
        sourceLabel.textContent = 'Source branch';
        this._sourceSelect = document.createElement('select');
        this._sourceSelect.className = 'gm-select';
        sourceRow.append(sourceLabel, this._sourceSelect);
        body.appendChild(sourceRow);

        // Strategy pills
        const stratRow = document.createElement('div');
        stratRow.className = 'gm-row';
        const stratLabel = document.createElement('span');
        stratLabel.className = 'gm-label';
        stratLabel.textContent = 'Strategy';
        const stratGroup = document.createElement('div');
        stratGroup.className = 'gm-strategy-group';

        this._stratBtns = {};
        for (const [key, label] of [['no-ff', '--no-ff'], ['ff', '--ff-only'], ['squash', '--squash']]) {
            const btn = document.createElement('button');
            btn.className = 'gm-strategy-btn';
            btn.textContent = label;
            btn.setAttribute('aria-pressed', key === this._strategy ? 'true' : 'false');
            btn.addEventListener('click', () => this._setStrategy(key));
            stratGroup.appendChild(btn);
            this._stratBtns[key] = btn;
        }
        stratRow.append(stratLabel, stratGroup);
        body.appendChild(stratRow);

        // Info box
        this._infoBox = document.createElement('div');
        this._infoBox.className = 'gm-info';
        this._infoBox.textContent = STRATEGY_INFO[this._strategy];
        body.appendChild(this._infoBox);

        // Merge button
        const btnRow = document.createElement('div');
        btnRow.className = 'gm-row';
        this._mergeBtn = document.createElement('button');
        this._mergeBtn.className = 'gm-btn';
        this._mergeBtn.textContent = '🔀 Merge';
        this._mergeBtn.disabled = true;
        this._mergeBtn.addEventListener('click', () => this._merge());
        btnRow.appendChild(this._mergeBtn);
        body.appendChild(btnRow);

        // Status + console
        this._status = document.createElement('div');
        this._status.className = 'gm-status';
        this._console = document.createElement('pre');
        this._console.className = 'gm-console';
        body.append(this._status, this._console);

        card.appendChild(body);
        wrap.appendChild(card);

        this._loadBranches();
    }

    _setStrategy(key) {
        this._strategy = key;
        for (const [k, btn] of Object.entries(this._stratBtns)) {
            btn.setAttribute('aria-pressed', k === key ? 'true' : 'false');
        }
        this._infoBox.textContent = STRATEGY_INFO[key];
    }

    async _loadBranches() {
        let data;
        try {
            const r = await fetch('/git/branches');
            data = await r.json();
        } catch {
            this._currentBadge.textContent = 'error';
            return;
        }
        if (!data.success) return;

        this._currentBranch = data.current;
        this._currentBadge.textContent = `⎇  ${data.current}`;

        this._sourceSelect.innerHTML = '';
        // Default empty option
        const placeholder = document.createElement('option');
        placeholder.value = '';
        placeholder.textContent = '— select source branch —';
        placeholder.disabled = true;
        placeholder.selected = true;
        this._sourceSelect.appendChild(placeholder);

        for (const b of data.local) {
            if (b === data.current) continue; // can't merge into itself
            const opt = document.createElement('option');
            opt.value = b;
            opt.textContent = b;
            this._sourceSelect.appendChild(opt);
        }

        if (data.remote?.length) {
            const sep = document.createElement('option');
            sep.disabled = true;
            sep.textContent = '── remote ──';
            this._sourceSelect.appendChild(sep);
            for (const b of data.remote) {
                if (b.endsWith('/HEAD')) continue;
                const opt = document.createElement('option');
                opt.value = b;
                opt.textContent = b;
                this._sourceSelect.appendChild(opt);
            }
        }

        this._sourceSelect.addEventListener('change', () => {
            this._mergeBtn.disabled = !this._sourceSelect.value;
        });
    }

    async _merge() {
        const source = this._sourceSelect.value;
        if (!source) return;

        this._mergeBtn.disabled = true;
        this._status.className = 'gm-status';
        this._console.className = 'gm-console';
        this._console.innerHTML = '';

        let data;
        try {
            const r = await fetch('/git/merge', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ branch: source, strategy: this._strategy }),
            });
            data = await r.json();
        } catch (e) {
            this._setStatus(false, 'Network error: ' + e.message);
            this._mergeBtn.disabled = false;
            return;
        }

        if (data.output) {
            this._console.classList.add('visible');
            const span = document.createElement('span');
            span.className = data.success ? 'ok' : 'err';
            span.textContent = data.output;
            this._console.appendChild(span);
        }

        if (data.success) {
            const note = this._strategy === 'squash'
                ? `Squash staged — commit the result manually.`
                : `Merged '${source}' into '${this._currentBranch}'.`;
            this._setStatus(true, note);
        } else {
            this._setStatus(false, data.error || 'Merge failed. Resolve conflicts and commit.');
        }

        this._mergeBtn.disabled = false;
    }

    _setStatus(ok, msg) {
        this._status.className = 'gm-status ' + (ok ? 'ok' : 'err');
        this._status.textContent = msg;
    }
}

customElements.define('git-merge', GitMerge);
