// Custom element <git-sub-repos>
// Displays the status of each repo in _sub_repos/.

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

/* ── Repo cards ──────────────────────────────────────── */
.repo-list {
    display: flex;
    flex-direction: column;
    gap: 0.75em;
}

.repo-card {
    border: 1px solid #e5e7eb;
    border-radius: 8px;
    overflow: hidden;
    background: #fff;
}

.repo-header {
    display: flex;
    align-items: center;
    gap: 0.8em;
    padding: 0.7em 1em;
    background: #f9fafb;
    border-bottom: 1px solid #e5e7eb;
    cursor: pointer;
    user-select: none;
    transition: background 0.1s;
}
.repo-header:hover { background: #f3f4f6; }
.repo-header[aria-expanded="true"] { background: #eff6ff; border-bottom-color: #bfdbfe; }

.repo-name {
    font-weight: 600;
    font-size: 0.95em;
    color: #1f2937;
    flex: 1;
}

.branch-badge {
    font-family: 'Cascadia Code', 'Fira Code', monospace;
    font-size: 0.78em;
    background: #d1fae5;
    color: #065f46;
    border-radius: 4px;
    padding: 0.15em 0.5em;
    white-space: nowrap;
}

.dirty-badge {
    font-size: 0.75em;
    background: #fef3c7;
    color: #92400e;
    border-radius: 4px;
    padding: 0.15em 0.5em;
    white-space: nowrap;
}

.clean-badge {
    font-size: 0.75em;
    background: #d1fae5;
    color: #065f46;
    border-radius: 4px;
    padding: 0.15em 0.5em;
    white-space: nowrap;
}

.toggle-icon {
    font-size: 0.75em;
    color: #6b7280;
    flex-shrink: 0;
    transition: transform 0.15s;
}
.repo-header[aria-expanded="true"] .toggle-icon { transform: rotate(90deg); }

/* ── Repo details ────────────────────────────────────── */
.repo-details {
    display: none;
    padding: 0.8em 1em;
    font-size: 0.85em;
}
.repo-details.open { display: block; }

.detail-row {
    display: flex;
    gap: 0.6em;
    padding: 0.25em 0;
    border-bottom: 1px solid #f3f4f6;
    align-items: baseline;
}
.detail-row:last-child { border-bottom: none; }

.detail-label {
    min-width: 80px;
    color: #6b7280;
    font-weight: 500;
    flex-shrink: 0;
}

.detail-value {
    color: #374151;
    word-break: break-all;
}

.sha-badge {
    font-family: 'Cascadia Code', 'Fira Code', monospace;
    font-size: 0.9em;
    color: #0e639c;
    background: #eff6ff;
    border-radius: 4px;
    padding: 0.1em 0.35em;
}

.path-label {
    color: #9ca3af;
    font-size: 0.85em;
    font-family: 'Cascadia Code', 'Fira Code', monospace;
}

.remote-entry {
    display: flex;
    gap: 0.4em;
    align-items: center;
    flex-wrap: wrap;
}

.remote-name {
    font-weight: 600;
    color: #4b5563;
    min-width: 60px;
}

.remote-url {
    color: #0e639c;
    font-size: 0.9em;
    word-break: break-all;
}
`;

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
        return d.toLocaleDateString(undefined, { year: 'numeric', month: 'short', day: 'numeric' })
            + ' ' + d.toLocaleTimeString(undefined, { hour: '2-digit', minute: '2-digit' });
    } catch { return iso; }
}

class GitSubRepos extends HTMLElement {
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
        this._content.innerHTML = '<div class="status">Loading sub-repos…</div>';
        try {
            const res = await fetch('/git/sub-repos');
            if (!res.ok) throw new Error(`HTTP ${res.status}`);
            const data = await res.json();
            if (!data.success) throw new Error(data.error || 'Unknown error');
            this._render(data.repos || []);
        } catch (err) {
            this._content.innerHTML = `<div class="status">Failed to load sub-repos: ${_esc(String(err))}</div>`;
        }
    }

    _render(repos) {
        this._countLabel.textContent = `${repos.length} sub-repo${repos.length !== 1 ? 's' : ''}`;

        if (repos.length === 0) {
            this._content.innerHTML = '<div class="status">No git repositories found in _sub_repos/.</div>';
            return;
        }

        const list = document.createElement('div');
        list.className = 'repo-list';

        repos.forEach(repo => {
            const card = document.createElement('div');
            card.className = 'repo-card';

            // Header
            const header = document.createElement('div');
            header.className = 'repo-header';
            header.setAttribute('role', 'button');
            header.setAttribute('aria-expanded', 'false');
            header.tabIndex = 0;

            const nameEl = document.createElement('span');
            nameEl.className = 'repo-name';
            nameEl.textContent = repo.name;

            const branchBadge = document.createElement('span');
            branchBadge.className = 'branch-badge';
            branchBadge.textContent = `⎇ ${repo.branch || 'unknown'}`;

            const statusBadge = document.createElement('span');
            statusBadge.className = repo.dirty ? 'dirty-badge' : 'clean-badge';
            statusBadge.textContent = repo.dirty
                ? `${repo.changed_files} change${repo.changed_files !== 1 ? 's' : ''}`
                : '✓ clean';

            const toggle = document.createElement('span');
            toggle.className = 'toggle-icon';
            toggle.textContent = '▶';

            header.appendChild(nameEl);
            header.appendChild(branchBadge);
            header.appendChild(statusBadge);
            header.appendChild(toggle);

            // Details panel
            const details = document.createElement('div');
            details.className = 'repo-details';

            const rows = [
                ['Path', `<span class="path-label">${_esc(repo.path)}</span>`],
                ['Commit', repo.sha ? `<span class="sha-badge">${_esc(repo.sha)}</span>` : '—'],
                ['Subject', _esc(repo.subject || '—')],
                ['Author', _esc(repo.author || '—')],
                ['Date', _esc(_formatDate(repo.date))],
            ];

            rows.forEach(([label, html]) => {
                const row = document.createElement('div');
                row.className = 'detail-row';
                row.innerHTML = `<span class="detail-label">${_esc(label)}</span><span class="detail-value">${html}</span>`;
                details.appendChild(row);
            });

            // Remotes
            const remoteEntries = Object.entries(repo.remotes || {});
            if (remoteEntries.length > 0) {
                const row = document.createElement('div');
                row.className = 'detail-row';
                const label = document.createElement('span');
                label.className = 'detail-label';
                label.textContent = 'Remotes';
                const val = document.createElement('span');
                val.className = 'detail-value';
                remoteEntries.forEach(([name, url]) => {
                    const entry = document.createElement('div');
                    entry.className = 'remote-entry';
                    entry.innerHTML = `<span class="remote-name">${_esc(name)}</span><span class="remote-url">${_esc(url)}</span>`;
                    val.appendChild(entry);
                });
                row.appendChild(label);
                row.appendChild(val);
                details.appendChild(row);
            }

            // Toggle expand
            const toggle_ = () => {
                const expanded = header.getAttribute('aria-expanded') === 'true';
                header.setAttribute('aria-expanded', expanded ? 'false' : 'true');
                details.classList.toggle('open', !expanded);
            };
            header.addEventListener('click', toggle_);
            header.addEventListener('keydown', e => { if (e.key === 'Enter' || e.key === ' ') toggle_(); });

            card.appendChild(header);
            card.appendChild(details);
            list.appendChild(card);
        });

        this._content.innerHTML = '';
        this._content.appendChild(list);
    }
}

customElements.define('git-sub-repos', GitSubRepos);
