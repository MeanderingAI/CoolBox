// Custom element <git-diff>
// Shows a colored diff for the working tree (git diff HEAD) or any commit (git show <sha>).

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Toolbar ─────────────────────────────────────────── */
.toolbar {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 0.6em;
    margin-bottom: 1em;
}

.ref-input {
    flex: 1 1 220px;
    padding: 0.38em 0.7em;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    font-size: 0.85em;
    font-family: 'Cascadia Code', 'Fira Code', monospace;
    outline: none;
    transition: border-color 0.15s;
}
.ref-input:focus { border-color: #0e639c; }

.ref-hint {
    font-size: 0.78em;
    color: #9ca3af;
    white-space: nowrap;
}

.load-btn {
    padding: 0.38em 1em;
    border: 1px solid #0e639c;
    border-radius: 6px;
    background: #0e639c;
    color: #fff;
    font-size: 0.85em;
    cursor: pointer;
    transition: background 0.15s;
}
.load-btn:hover { background: #0a4f7e; }

/* ── Status ──────────────────────────────────────────── */
.status {
    padding: 2em;
    text-align: center;
    color: #6b7280;
    font-size: 0.9em;
}

/* ── Diff output ─────────────────────────────────────── */
.diff-wrap {
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    overflow: hidden;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace;
    font-size: 0.82em;
}

.diff-file-header {
    background: #1e1e2e;
    color: #c0caf5;
    padding: 0.4em 0.8em;
    font-weight: 600;
    border-top: 2px solid #2a2a3e;
    letter-spacing: 0.02em;
}
.diff-file-header:first-child { border-top: none; }

.diff-lines { background: #0d1117; }

.diff-line {
    display: flex;
    min-height: 1.4em;
    line-height: 1.5;
    white-space: pre;
}

.diff-ln {
    min-width: 3em;
    text-align: right;
    padding: 0 0.5em;
    color: #4b5563;
    user-select: none;
    border-right: 1px solid #21262d;
    flex-shrink: 0;
}

.diff-text {
    padding: 0 0.6em;
    flex: 1;
    overflow-x: auto;
    color: #c9d1d9;
    white-space: pre;
}

.diff-line.add    .diff-text { background: #0d2b0a; color: #7ee787; }
.diff-line.add    .diff-ln   { background: #0d2b0a; }
.diff-line.remove .diff-text { background: #2b0a0a; color: #f97583; }
.diff-line.remove .diff-ln   { background: #2b0a0a; }
.diff-line.hunk   .diff-text { background: #0c2040; color: #79c0ff; }
.diff-line.hunk   .diff-ln   { background: #0c2040; }

/* ── Summary bar ─────────────────────────────────────── */
.summary {
    display: flex;
    gap: 1em;
    padding: 0.4em 0.8em;
    font-size: 0.8em;
    color: #6b7280;
    background: #f9fafb;
    border-top: 1px solid #e5e7eb;
}
.add-count { color: #16a34a; font-weight: 600; }
.del-count { color: #dc2626; font-weight: 600; }
`;

function _esc(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
}

function _parseDiff(raw) {
    // Split into file sections and line-annotated rows
    const sections = [];
    let current = null;
    let lineNum = 0;

    for (const line of raw.split('\n')) {
        if (line.startsWith('diff --git') || line.startsWith('--- ') && current === null) {
            // New file section heading
            if (line.startsWith('diff --git')) {
                current = { header: line.replace('diff --git ', ''), lines: [] };
                sections.push(current);
                lineNum = 0;
            }
        } else if (line.startsWith('+++ ') || line.startsWith('--- ')) {
            // part of the file header, attach to current section header
            if (current) current.header = line.replace(/^[+-]{3} /, '');
        } else if (line.startsWith('@@')) {
            // Hunk header — extract starting line number
            const m = line.match(/@@ -\d+(?:,\d+)? \+(\d+)/);
            lineNum = m ? parseInt(m[1], 10) - 1 : lineNum;
            if (current) current.lines.push({ type: 'hunk', ln: '', text: line });
        } else if (line.startsWith('+')) {
            lineNum++;
            if (current) current.lines.push({ type: 'add', ln: lineNum, text: line.slice(1) });
        } else if (line.startsWith('-')) {
            if (current) current.lines.push({ type: 'remove', ln: '', text: line.slice(1) });
        } else {
            lineNum++;
            if (current) current.lines.push({ type: 'ctx', ln: lineNum, text: line.slice(1) });
        }
    }
    return sections;
}

class GitDiff extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';

        this._refInput = document.createElement('input');
        this._refInput.className = 'ref-input';
        this._refInput.placeholder = 'Commit SHA (empty = working tree vs HEAD)';
        this._refInput.setAttribute('aria-label', 'Commit reference');

        const hint = document.createElement('span');
        hint.className = 'ref-hint';
        hint.textContent = 'SHA or empty for staged+unstaged';

        const loadBtn = document.createElement('button');
        loadBtn.className = 'load-btn';
        loadBtn.textContent = 'Show Diff';
        loadBtn.addEventListener('click', () => this._load());

        this._refInput.addEventListener('keydown', e => { if (e.key === 'Enter') this._load(); });

        toolbar.appendChild(this._refInput);
        toolbar.appendChild(hint);
        toolbar.appendChild(loadBtn);
        shadow.appendChild(toolbar);

        this._content = document.createElement('div');
        shadow.appendChild(this._content);

        // Auto-load working tree diff on mount
        this._load();
    }

    async _load() {
        const ref = this._refInput.value.trim();
        this._content.innerHTML = '<div class="status">Loading diff…</div>';

        const params = new URLSearchParams();
        if (ref) params.set('ref', ref);

        try {
            const res = await fetch(`/git/diff?${params}`);
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (!data.success) throw new Error(data.error || 'Unknown error');
            this._render(data.diff || '', data.ref || '');
        } catch (err) {
            this._content.innerHTML = `<div class="status">Failed: ${_esc(String(err))}</div>`;
        }
    }

    _render(raw, ref) {
        if (!raw.trim()) {
            this._content.innerHTML = `<div class="status">No changes — working tree is clean.</div>`;
            return;
        }

        const sections = _parseDiff(raw);
        let totalAdds = 0, totalDels = 0;

        const wrap = document.createElement('div');
        wrap.className = 'diff-wrap';

        for (const sec of sections) {
            const fileHdr = document.createElement('div');
            fileHdr.className = 'diff-file-header';
            fileHdr.textContent = sec.header;
            wrap.appendChild(fileHdr);

            const linesDiv = document.createElement('div');
            linesDiv.className = 'diff-lines';

            for (const { type, ln, text } of sec.lines) {
                if (type === 'add') totalAdds++;
                if (type === 'remove') totalDels++;

                const row = document.createElement('div');
                row.className = `diff-line ${type}`;

                const lnDiv = document.createElement('div');
                lnDiv.className = 'diff-ln';
                lnDiv.textContent = ln || '';

                const textDiv = document.createElement('div');
                textDiv.className = 'diff-text';
                textDiv.textContent = text;

                row.appendChild(lnDiv);
                row.appendChild(textDiv);
                linesDiv.appendChild(row);
            }

            wrap.appendChild(linesDiv);
        }

        const summary = document.createElement('div');
        summary.className = 'summary';
        const refLabel = ref ? `Showing: ${ref}` : 'Showing: working tree vs HEAD';
        summary.innerHTML = `<span>${_esc(refLabel)}</span>
            <span class="add-count">+${totalAdds}</span>
            <span class="del-count">-${totalDels}</span>`;
        wrap.appendChild(summary);

        this._content.innerHTML = '';
        this._content.appendChild(wrap);
    }
}

customElements.define('git-diff', GitDiff);
