// Custom element <git-stack>
// Displays the git stash list with expandable diffs for each stash entry.

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Toolbar ─────────────────────────────────────────── */
.toolbar {
    display: flex;
    align-items: center;
    gap: 0.6em;
    margin-bottom: 1em;
    flex-wrap: wrap;
}

.refresh-btn {
    padding: 0.35em 0.8em;
    border: 1px solid #0e639c;
    border-radius: 6px;
    background: transparent;
    color: #0e639c;
    font-size: 0.82em;
    cursor: pointer;
    transition: background 0.15s, color 0.15s;
}
.refresh-btn:hover { background: #0e639c; color: #fff; }

.count-label {
    font-size: 0.8em;
    color: #6b7280;
}

/* ── Status ──────────────────────────────────────────── */
.status {
    padding: 2em;
    text-align: center;
    color: #6b7280;
    font-size: 0.9em;
}

/* ── Stash list ──────────────────────────────────────── */
.stash-list {
    display: flex;
    flex-direction: column;
    gap: 0.5em;
}

.stash-entry {
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    overflow: hidden;
}

.stash-header {
    display: flex;
    align-items: center;
    gap: 0.8em;
    padding: 0.6em 0.9em;
    background: #f9fafb;
    cursor: pointer;
    user-select: none;
    transition: background 0.1s;
}
.stash-header:hover { background: #f3f4f6; }
.stash-header[aria-expanded="true"] { background: #eff6ff; border-bottom: 1px solid #bfdbfe; }

.stash-idx {
    font-family: 'Cascadia Code', 'Fira Code', monospace;
    font-size: 0.8em;
    color: #0e639c;
    background: #dbeafe;
    border-radius: 4px;
    padding: 0.1em 0.45em;
    white-space: nowrap;
    flex-shrink: 0;
}

.stash-msg {
    flex: 1;
    font-size: 0.88em;
    color: #374151;
    word-break: break-word;
}

.stash-date {
    font-size: 0.78em;
    color: #9ca3af;
    white-space: nowrap;
}

.toggle-icon {
    font-size: 0.75em;
    color: #6b7280;
    flex-shrink: 0;
    transition: transform 0.15s;
}
.stash-header[aria-expanded="true"] .toggle-icon { transform: rotate(90deg); }

/* ── Stash diff ──────────────────────────────────────── */
.stash-diff {
    display: none;
    background: #0d1117;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace;
    font-size: 0.8em;
    max-height: 500px;
    overflow-y: auto;
}
.stash-diff.open { display: block; }

.diff-file-header {
    background: #1e1e2e;
    color: #c0caf5;
    padding: 0.35em 0.8em;
    font-weight: 600;
    border-top: 1px solid #2a2a3e;
}

.diff-line {
    display: flex;
    min-height: 1.35em;
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
.diff-text { padding: 0 0.6em; flex: 1; color: #c9d1d9; }
.diff-line.add    .diff-text { background: #0d2b0a; color: #7ee787; }
.diff-line.add    .diff-ln   { background: #0d2b0a; }
.diff-line.remove .diff-text { background: #2b0a0a; color: #f97583; }
.diff-line.remove .diff-ln   { background: #2b0a0a; }
.diff-line.hunk   .diff-text { background: #0c2040; color: #79c0ff; }
.diff-line.hunk   .diff-ln   { background: #0c2040; }

.diff-loading { padding: 1em; color: #6b7280; font-family: inherit; }
`;

function _esc(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
}

function _renderDiffLines(container, raw) {
    let lineNum = 0;
    let currentFile = null;
    let linesDiv = null;

    for (const line of raw.split('\n')) {
        if (line.startsWith('diff --git')) {
            const hdr = document.createElement('div');
            hdr.className = 'diff-file-header';
            hdr.textContent = line.replace('diff --git ', '');
            container.appendChild(hdr);
            linesDiv = document.createElement('div');
            container.appendChild(linesDiv);
            lineNum = 0;
            currentFile = hdr;
        } else if (line.startsWith('@@')) {
            const m = line.match(/@@ -\d+(?:,\d+)? \+(\d+)/);
            lineNum = m ? parseInt(m[1], 10) - 1 : lineNum;
            const row = _makeRow('hunk', '', line);
            linesDiv?.appendChild(row);
        } else if (line.startsWith('+') && !line.startsWith('+++')) {
            lineNum++;
            linesDiv?.appendChild(_makeRow('add', lineNum, line.slice(1)));
        } else if (line.startsWith('-') && !line.startsWith('---')) {
            linesDiv?.appendChild(_makeRow('remove', '', line.slice(1)));
        } else if (linesDiv && line.startsWith(' ')) {
            lineNum++;
            linesDiv.appendChild(_makeRow('ctx', lineNum, line.slice(1)));
        }
    }
}

function _makeRow(type, ln, text) {
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
    return row;
}

class GitStack extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';

        this._countLabel = document.createElement('span');
        this._countLabel.className = 'count-label';

        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'refresh-btn';
        refreshBtn.textContent = '↺ Refresh';
        refreshBtn.addEventListener('click', () => this._load());

        toolbar.appendChild(this._countLabel);
        toolbar.appendChild(refreshBtn);
        shadow.appendChild(toolbar);

        this._content = document.createElement('div');
        shadow.appendChild(this._content);

        this._load();
    }

    async _load() {
        this._content.innerHTML = '<div class="status">Loading stash…</div>';
        try {
            const res = await fetch('/git/stash');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (!data.success) throw new Error(data.error || 'Unknown error');
            this._render(data.entries || []);
        } catch (err) {
            this._content.innerHTML = `<div class="status">Failed: ${_esc(String(err))}</div>`;
        }
    }

    _render(entries) {
        this._countLabel.textContent = `${entries.length} stash entr${entries.length !== 1 ? 'ies' : 'y'}`;

        if (entries.length === 0) {
            this._content.innerHTML = '<div class="status">Stash is empty — no saved changes.</div>';
            return;
        }

        const list = document.createElement('div');
        list.className = 'stash-list';

        entries.forEach((entry, i) => {
            const card = document.createElement('div');
            card.className = 'stash-entry';

            const header = document.createElement('div');
            header.className = 'stash-header';
            header.setAttribute('role', 'button');
            header.setAttribute('aria-expanded', 'false');
            header.tabIndex = 0;

            const idxBadge = document.createElement('span');
            idxBadge.className = 'stash-idx';
            idxBadge.textContent = `stash@{${i}}`;

            const msg = document.createElement('span');
            msg.className = 'stash-msg';
            msg.textContent = entry.message;

            const date = document.createElement('span');
            date.className = 'stash-date';
            date.textContent = entry.date || '';

            const toggle = document.createElement('span');
            toggle.className = 'toggle-icon';
            toggle.textContent = '▶';

            header.appendChild(idxBadge);
            header.appendChild(msg);
            header.appendChild(date);
            header.appendChild(toggle);

            const diffDiv = document.createElement('div');
            diffDiv.className = 'stash-diff';
            let loaded = false;

            const expand = async () => {
                const expanded = header.getAttribute('aria-expanded') === 'true';
                if (expanded) {
                    header.setAttribute('aria-expanded', 'false');
                    diffDiv.classList.remove('open');
                    return;
                }
                header.setAttribute('aria-expanded', 'true');
                diffDiv.classList.add('open');
                if (loaded) return;
                loaded = true;
                diffDiv.innerHTML = '<div class="diff-loading">Loading diff…</div>';
                try {
                    const res = await fetch(`/git/stash/${i}`);
                    if (!res.ok) throw new Error(`HTTP ${res.status}`);
                    const data = await res.json();
                    diffDiv.innerHTML = '';
                    if (data.diff) {
                        _renderDiffLines(diffDiv, data.diff);
                    } else {
                        diffDiv.innerHTML = '<div class="diff-loading">No diff available.</div>';
                    }
                } catch (err) {
                    diffDiv.innerHTML = `<div class="diff-loading" style="color:#f97583">Error: ${_esc(String(err))}</div>`;
                }
            };

            header.addEventListener('click', expand);
            header.addEventListener('keydown', e => { if (e.key === 'Enter' || e.key === ' ') expand(); });

            card.appendChild(header);
            card.appendChild(diffDiv);
            list.appendChild(card);
        });

        this._content.innerHTML = '';
        this._content.appendChild(list);
    }
}

customElements.define('git-stack', GitStack);
