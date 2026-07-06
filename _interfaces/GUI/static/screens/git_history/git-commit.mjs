// Custom element <git-commit>
// Allows switching branches and creating commits (git add -A + git commit).

const STYLE = `
:host { display: block; font-family: inherit; }

.gc-wrap { padding: 1em; display: flex; flex-direction: column; gap: 1.2em; }

/* ── Section card ───────────────────────────────────────── */
.gc-card {
    background: #f8faff;
    border: 1px solid #dde1ea;
    border-radius: 8px;
    overflow: hidden;
}

.gc-card-header {
    padding: 0.5em 0.9em;
    background: #f0f2f8;
    border-bottom: 1px solid #dde1ea;
    font-size: 0.75em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6b7280;
}

.gc-card-body { padding: 0.8em 1em; }

/* ── Branch switcher ────────────────────────────────────── */
.gc-branch-row {
    display: flex;
    gap: 0.6em;
    align-items: center;
    flex-wrap: wrap;
}

.gc-current-badge {
    display: inline-block;
    padding: 0.25em 0.7em;
    background: #dbeafe;
    border: 1px solid #93c5fd;
    border-radius: 4px;
    font-size: 0.82em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #1d4ed8;
    font-weight: 700;
}

.gc-select {
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
.gc-select:focus { outline: 2px solid #0e639c; outline-offset: 1px; }

/* ── Buttons ────────────────────────────────────────────── */
.gc-btn {
    padding: 0.35em 1em;
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
.gc-btn:hover  { background: #1177bb; }
.gc-btn:disabled { background: #9ca3af; cursor: default; }
.gc-btn.danger { background: #dc2626; }
.gc-btn.danger:hover { background: #b91c1c; }

/* ── Commit form ────────────────────────────────────────── */
.gc-label {
    display: block;
    font-size: 0.78em;
    font-weight: 700;
    letter-spacing: 0.05em;
    text-transform: uppercase;
    color: #6b7280;
    margin-bottom: 0.3em;
}

.gc-textarea {
    width: 100%;
    box-sizing: border-box;
    min-height: 70px;
    padding: 0.4em 0.6em;
    font-size: 0.88em;
    font-family: inherit;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    resize: vertical;
    color: #1f2937;
}
.gc-textarea:focus { outline: 2px solid #0e639c; outline-offset: 1px; }

.gc-commit-row {
    display: flex;
    gap: 0.6em;
    align-items: center;
    margin-top: 0.6em;
    flex-wrap: wrap;
}

.gc-char-count {
    font-size: 0.76em;
    color: #9ca3af;
    margin-left: auto;
}
.gc-char-count.warn { color: #d97706; }

/* ── Output console ─────────────────────────────────────── */
.gc-console {
    margin-top: 0.5em;
    background: #1e1e2e;
    color: #d4d4d4;
    border-radius: 5px;
    padding: 0.6em 0.8em;
    font-size: 0.8em;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    white-space: pre-wrap;
    word-break: break-all;
    max-height: 180px;
    overflow-y: auto;
    display: none;
}
.gc-console.visible { display: block; }
.gc-console .ok  { color: #4ec9b0; }
.gc-console .err { color: #f48771; }

/* ── Status banner ──────────────────────────────────────── */
.gc-status {
    padding: 0.4em 0.8em;
    border-radius: 5px;
    font-size: 0.82em;
    display: none;
    margin-top: 0.4em;
}
.gc-status.ok  { background: #d1fae5; border: 1px solid #6ee7b7; color: #065f46; display: block; }
.gc-status.err { background: #fef2f2; border: 1px solid #fca5a5; color: #dc2626; display: block; }
`;

class GitCommit extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        shadow.innerHTML = `<style>${STYLE}</style>`;

        const wrap = document.createElement('div');
        wrap.className = 'gc-wrap';
        shadow.appendChild(wrap);

        // ── Branch Switcher card ──────────────────────────────
        const branchCard = document.createElement('div');
        branchCard.className = 'gc-card';
        branchCard.innerHTML = `<div class="gc-card-header">🌿 Switch Branch</div>`;
        const branchBody = document.createElement('div');
        branchBody.className = 'gc-card-body';

        this._currentBadge = document.createElement('span');
        this._currentBadge.className = 'gc-current-badge';
        this._currentBadge.textContent = '…';

        this._branchSelect = document.createElement('select');
        this._branchSelect.className = 'gc-select';

        this._checkoutBtn = document.createElement('button');
        this._checkoutBtn.className = 'gc-btn';
        this._checkoutBtn.textContent = '✔ Checkout';
        this._checkoutBtn.disabled = true;
        this._checkoutBtn.addEventListener('click', () => this._checkout(shadow));

        this._branchSelect.addEventListener('change', () => {
            const selected = this._branchSelect.value;
            this._checkoutBtn.disabled = selected === this._currentBranch || !selected;
        });

        const branchRow = document.createElement('div');
        branchRow.className = 'gc-branch-row';
        branchRow.append(this._currentBadge, this._branchSelect, this._checkoutBtn);
        branchBody.appendChild(branchRow);

        this._branchStatus = document.createElement('div');
        this._branchStatus.className = 'gc-status';
        branchBody.appendChild(this._branchStatus);

        this._branchConsole = document.createElement('pre');
        this._branchConsole.className = 'gc-console';
        branchBody.appendChild(this._branchConsole);

        branchCard.appendChild(branchBody);
        wrap.appendChild(branchCard);

        // ── Commit card ──────────────────────────────────────
        const commitCard = document.createElement('div');
        commitCard.className = 'gc-card';
        commitCard.innerHTML = `<div class="gc-card-header">📝 Create Commit</div>`;
        const commitBody = document.createElement('div');
        commitBody.className = 'gc-card-body';

        const label = document.createElement('label');
        label.className = 'gc-label';
        label.textContent = 'Commit message';

        this._msgArea = document.createElement('textarea');
        this._msgArea.className = 'gc-textarea';
        this._msgArea.placeholder = 'Describe your changes…';

        this._charCount = document.createElement('span');
        this._charCount.className = 'gc-char-count';
        this._charCount.textContent = '0 chars';

        this._msgArea.addEventListener('input', () => {
            const len = this._msgArea.value.length;
            this._charCount.textContent = `${len} chars`;
            this._charCount.classList.toggle('warn', len > 72);
        });

        this._commitBtn = document.createElement('button');
        this._commitBtn.className = 'gc-btn';
        this._commitBtn.textContent = '🔒 Stage all & Commit';
        this._commitBtn.addEventListener('click', () => this._commit(shadow));

        const commitRow = document.createElement('div');
        commitRow.className = 'gc-commit-row';
        commitRow.append(this._commitBtn, this._charCount);

        this._commitStatus = document.createElement('div');
        this._commitStatus.className = 'gc-status';

        this._commitConsole = document.createElement('pre');
        this._commitConsole.className = 'gc-console';

        commitBody.append(label, this._msgArea, commitRow, this._commitStatus, this._commitConsole);
        commitCard.appendChild(commitBody);
        wrap.appendChild(commitCard);

        // ── Load branches ────────────────────────────────────
        this._loadBranches();
    }

    async _loadBranches() {
        let data;
        try {
            const r = await fetch('/git/branches');
            data = await r.json();
        } catch (e) {
            this._currentBadge.textContent = 'error';
            return;
        }
        if (!data.success) return;

        this._currentBranch = data.current;
        this._currentBadge.textContent = `⎇  ${data.current}`;

        this._branchSelect.innerHTML = '';
        for (const b of data.local) {
            const opt = document.createElement('option');
            opt.value = b;
            opt.textContent = b;
            if (b === data.current) opt.selected = true;
            this._branchSelect.appendChild(opt);
        }

        // Separator + remote tracking branches
        if (data.remote.length) {
            const sep = document.createElement('option');
            sep.disabled = true;
            sep.textContent = '── remote ──';
            this._branchSelect.appendChild(sep);
            for (const b of data.remote) {
                // Skip HEAD
                if (b.endsWith('/HEAD')) continue;
                const opt = document.createElement('option');
                opt.value = b;
                opt.textContent = b;
                this._branchSelect.appendChild(opt);
            }
        }

        this._checkoutBtn.disabled = true;
    }

    async _checkout(shadow) {
        const branch = this._branchSelect.value;
        if (!branch) return;

        this._checkoutBtn.disabled = true;
        this._branchStatus.className = 'gc-status';
        this._branchConsole.className = 'gc-console';
        this._branchConsole.textContent = '';

        let data;
        try {
            const r = await fetch('/git/checkout', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ branch }),
            });
            data = await r.json();
        } catch (e) {
            this._setStatus(this._branchStatus, false, 'Network error: ' + e.message);
            this._checkoutBtn.disabled = false;
            return;
        }

        if (data.output) {
            this._branchConsole.classList.add('visible');
            const inner = document.createElement('span');
            inner.className = data.success ? 'ok' : 'err';
            inner.textContent = data.output;
            this._branchConsole.appendChild(inner);
        }

        if (data.success) {
            this._setStatus(this._branchStatus, true, `Switched to branch '${branch}'`);
            this._currentBranch = branch;
            this._currentBadge.textContent = `⎇  ${branch}`;
            this._checkoutBtn.disabled = true;
        } else {
            this._setStatus(this._branchStatus, false, data.error || 'Checkout failed');
            this._checkoutBtn.disabled = false;
        }
    }

    async _commit(shadow) {
        const message = this._msgArea.value.trim();
        if (!message) {
            this._setStatus(this._commitStatus, false, 'Please enter a commit message.');
            return;
        }

        this._commitBtn.disabled = true;
        this._commitStatus.className = 'gc-status';
        this._commitConsole.className = 'gc-console';
        this._commitConsole.textContent = '';

        let data;
        try {
            const r = await fetch('/git/commit', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ message }),
            });
            data = await r.json();
        } catch (e) {
            this._setStatus(this._commitStatus, false, 'Network error: ' + e.message);
            this._commitBtn.disabled = false;
            return;
        }

        if (data.output) {
            this._commitConsole.classList.add('visible');
            const inner = document.createElement('span');
            inner.className = data.success ? 'ok' : 'err';
            inner.textContent = data.output;
            this._commitConsole.appendChild(inner);
        }

        if (data.success) {
            this._setStatus(this._commitStatus, true, 'Commit created successfully.');
            this._msgArea.value = '';
            this._charCount.textContent = '0 chars';
            this._charCount.classList.remove('warn');
        } else {
            this._setStatus(this._commitStatus, false, data.error || 'Commit failed.');
        }

        this._commitBtn.disabled = false;
    }

    _setStatus(el, ok, msg) {
        el.className = 'gc-status ' + (ok ? 'ok' : 'err');
        el.textContent = msg;
    }
}

customElements.define('git-commit', GitCommit);
