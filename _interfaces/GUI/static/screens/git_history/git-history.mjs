// Custom element <git-history>
// Displays a git log table with branch switcher and search filter.

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

.branch-select {
    padding: 0.35em 0.6em;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    font-size: 0.85em;
    background: #f9fafb;
    cursor: pointer;
}

.search-input {
    flex: 1 1 200px;
    padding: 0.35em 0.7em;
    border: 1px solid #d1d5db;
    border-radius: 6px;
    font-size: 0.85em;
    outline: none;
    transition: border-color 0.15s;
}
.search-input:focus { border-color: #0e639c; }

.count-label {
    font-size: 0.8em;
    color: #6b7280;
    white-space: nowrap;
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

/* ── Status ──────────────────────────────────────────── */
.status {
    padding: 2em;
    text-align: center;
    color: #6b7280;
    font-size: 0.9em;
}

/* ── Table ───────────────────────────────────────────── */
.table-wrap {
    overflow-x: auto;
    border-radius: 8px;
    border: 1px solid #e5e7eb;
}

table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.85em;
}

thead th {
    background: #f3f4f6;
    text-align: left;
    padding: 0.55em 0.8em;
    font-weight: 600;
    color: #374151;
    border-bottom: 1px solid #e5e7eb;
    white-space: nowrap;
    user-select: none;
}

tbody tr {
    border-bottom: 1px solid #f3f4f6;
    transition: background 0.1s;
}
tbody tr:last-child { border-bottom: none; }
tbody tr:hover { background: #f9fafb; }

td {
    padding: 0.48em 0.8em;
    vertical-align: top;
    color: #374151;
}

/* ── SHA badge ───────────────────────────────────────── */
.sha {
    font-family: 'Cascadia Code', 'Fira Code', monospace;
    font-size: 0.88em;
    color: #0e639c;
    background: #eff6ff;
    border-radius: 4px;
    padding: 0.1em 0.35em;
    white-space: nowrap;
}

/* ── Subject ─────────────────────────────────────────── */
.subject { max-width: 480px; word-break: break-word; }
mark { background: #fde68a; border-radius: 2px; }

/* ── Refs (branches / tags) ──────────────────────────── */
.refs { display: flex; flex-wrap: wrap; gap: 0.3em; }
.ref-tag {
    display: inline-block;
    font-size: 0.75em;
    border-radius: 4px;
    padding: 0.1em 0.45em;
    white-space: nowrap;
}
.ref-branch { background: #d1fae5; color: #065f46; }
.ref-tag-name { background: #fef3c7; color: #92400e; }

/* ── Date ────────────────────────────────────────────── */
.date { white-space: nowrap; color: #6b7280; font-size: 0.82em; }

/* ── Author ──────────────────────────────────────────── */
.author { white-space: nowrap; font-size: 0.82em; color: #4b5563; }
`;

function _highlight(text, query) {
    if (!query) return _esc(text);
    const escaped = query.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    return _esc(text).replace(new RegExp(`(${escaped})`, 'gi'), '<mark>$1</mark>');
}

function _esc(s) {
    return String(s)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
}

function _formatDate(iso) {
    if (!iso) return '';
    try {
        const d = new Date(iso);
        if (isNaN(d.getTime())) return iso;
        return d.toLocaleDateString(undefined, { year: 'numeric', month: 'short', day: 'numeric' })
            + ' ' + d.toLocaleTimeString(undefined, { hour: '2-digit', minute: '2-digit' });
    } catch { return iso; }
}

class GitHistory extends HTMLElement {
    constructor() {
        super();
        this._commits = [];
        this._branches = [];
        this._query = '';
        this._branch = '';
    }

    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Toolbar
        const toolbar = document.createElement('div');
        toolbar.className = 'toolbar';

        this._branchSelect = document.createElement('select');
        this._branchSelect.className = 'branch-select';
        this._branchSelect.innerHTML = '<option value="">All branches</option>';
        this._branchSelect.addEventListener('change', () => {
            this._branch = this._branchSelect.value;
            this._load();
        });

        this._searchInput = document.createElement('input');
        this._searchInput.type = 'search';
        this._searchInput.className = 'search-input';
        this._searchInput.placeholder = 'Filter commits…';
        this._searchInput.addEventListener('input', () => {
            this._query = this._searchInput.value.trim();
            this._render();
        });

        this._countLabel = document.createElement('span');
        this._countLabel.className = 'count-label';

        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'refresh-btn';
        refreshBtn.textContent = '↺ Refresh';
        refreshBtn.addEventListener('click', () => this._load());

        toolbar.appendChild(this._branchSelect);
        toolbar.appendChild(this._searchInput);
        toolbar.appendChild(this._countLabel);
        toolbar.appendChild(refreshBtn);
        shadow.appendChild(toolbar);

        // Content area
        this._content = document.createElement('div');
        shadow.appendChild(this._content);

        this._load();
    }

    async _load() {
        this._content.innerHTML = '<div class="status">Loading…</div>';
        const params = new URLSearchParams({ n: 100 });
        if (this._branch) params.set('branch', this._branch);
        try {
            const res = await fetch(`/git/log?${params}`);
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (!data.success) throw new Error(data.error || 'Unknown error');
            this._commits = data.commits || [];
            this._branches = data.branches || [];
            this._populateBranches();
            this._render();
        } catch (err) {
            this._content.innerHTML = `<div class="status">Failed to load git log: ${_esc(String(err))}</div>`;
        }
    }

    _populateBranches() {
        const sel = this._branchSelect;
        const current = sel.value;
        // Keep "All branches" option, rebuild rest
        while (sel.options.length > 1) sel.remove(1);
        for (const b of this._branches) {
            const opt = document.createElement('option');
            opt.value = b;
            opt.textContent = b;
            if (b === current) opt.selected = true;
            sel.appendChild(opt);
        }
    }

    _render() {
        const q = this._query.toLowerCase();
        const filtered = q
            ? this._commits.filter(c =>
                c.subject.toLowerCase().includes(q) ||
                c.author.toLowerCase().includes(q) ||
                c.short.toLowerCase().includes(q) ||
                c.branches.some(b => b.toLowerCase().includes(q)) ||
                c.tags.some(t => t.toLowerCase().includes(q))
              )
            : this._commits;

        this._countLabel.textContent = `${filtered.length} commit${filtered.length !== 1 ? 's' : ''}`;

        if (filtered.length === 0) {
            this._content.innerHTML = '<div class="status">No commits match.</div>';
            return;
        }

        const rows = filtered.map(c => {
            const refHtml = [
                ...c.branches.map(b => `<span class="ref-tag ref-branch">${_esc(b)}</span>`),
                ...c.tags.map(t => `<span class="ref-tag ref-tag-name">🏷 ${_esc(t)}</span>`),
            ].join('');

            return `<tr>
                <td><span class="sha">${_esc(c.short)}</span></td>
                <td class="subject">${_highlight(c.subject, this._query)}${refHtml ? `<br><span class="refs">${refHtml}</span>` : ''}</td>
                <td class="author">${_highlight(c.author, this._query)}</td>
                <td class="date">${_formatDate(c.date)}</td>
            </tr>`;
        }).join('');

        this._content.innerHTML = `
            <div class="table-wrap">
                <table>
                    <thead>
                        <tr>
                            <th>SHA</th>
                            <th>Subject</th>
                            <th>Author</th>
                            <th>Date</th>
                        </tr>
                    </thead>
                    <tbody>${rows}</tbody>
                </table>
            </div>`;
    }
}

customElements.define('git-history', GitHistory);
