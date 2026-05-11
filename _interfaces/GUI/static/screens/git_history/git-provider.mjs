// Custom element <git-provider>
// Shows all git remotes (origin, upstream, etc.) with their URLs.

const STYLE = `
:host { display: block; font-family: inherit; }

.gp-wrap { padding: 1em; }

.gp-title {
    font-size: 0.75em;
    font-weight: 700;
    letter-spacing: 0.07em;
    text-transform: uppercase;
    color: #6b7280;
    margin-bottom: 0.8em;
}

/* ── Remote table ──────────────────────────────────────── */
.gp-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.85em;
}

.gp-table th {
    text-align: left;
    padding: 0.4em 0.7em;
    background: #f0f2f8;
    border: 1px solid #dde1ea;
    color: #374151;
    font-size: 0.8em;
    font-weight: 700;
    letter-spacing: 0.05em;
    text-transform: uppercase;
}

.gp-table td {
    padding: 0.5em 0.7em;
    border: 1px solid #e5e7eb;
    vertical-align: top;
    color: #1f2937;
}

.gp-table tr:hover td { background: #f8faff; }

.remote-name {
    font-weight: 700;
    font-family: 'Cascadia Code', 'Consolas', monospace;
    color: #0e639c;
    white-space: nowrap;
}

.url-cell {
    font-family: 'Cascadia Code', 'Consolas', monospace;
    font-size: 0.88em;
    word-break: break-all;
    color: #374151;
}

.url-sep {
    font-size: 0.72em;
    color: #9ca3af;
    display: block;
    margin-top: 0.2em;
}

/* ── Empty / loading states ─────────────────────────────── */
.gp-empty {
    padding: 1.5em;
    text-align: center;
    color: #9ca3af;
    font-style: italic;
    font-size: 0.88em;
}

.gp-error {
    padding: 0.6em 0.9em;
    background: #fef2f2;
    border: 1px solid #fca5a5;
    border-radius: 6px;
    color: #dc2626;
    font-size: 0.85em;
    margin-top: 0.5em;
}

.gp-refresh {
    float: right;
    padding: 0.3em 0.9em;
    font-size: 0.8em;
    font-family: inherit;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 5px;
    cursor: pointer;
    color: #374151;
    transition: background 0.12s;
}
.gp-refresh:hover { background: #f0f2f8; }
`;

class GitProvider extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        shadow.innerHTML = `<style>${STYLE}</style>`;

        const wrap = document.createElement('div');
        wrap.className = 'gp-wrap';

        const header = document.createElement('div');
        header.style.cssText = 'display:flex;align-items:center;margin-bottom:0.8em;';
        header.innerHTML = `<div class="gp-title">Git Remotes</div>`;
        const refreshBtn = document.createElement('button');
        refreshBtn.className = 'gp-refresh';
        refreshBtn.textContent = '↻ Refresh';
        refreshBtn.addEventListener('click', () => this._load(shadow, wrap));
        header.appendChild(refreshBtn);

        wrap.appendChild(header);
        shadow.appendChild(wrap);

        this._load(shadow, wrap);
    }

    async _load(shadow, wrap) {
        // Remove old table / error / empty
        ['gp-tbl-wrap', 'gp-err', 'gp-mt'].forEach(id => {
            const el = wrap.querySelector(`[data-id="${id}"]`);
            if (el) el.remove();
        });

        let data;
        try {
            const r = await fetch('/git/remotes');
            data = await r.json();
        } catch (e) {
            this._showError(wrap, 'Network error: ' + e.message);
            return;
        }

        if (!data.success) {
            this._showError(wrap, data.error || 'Failed to load remotes');
            return;
        }

        if (!data.remotes.length) {
            const mt = document.createElement('div');
            mt.className = 'gp-empty';
            mt.dataset.id = 'gp-mt';
            mt.textContent = 'No remotes configured for this repository.';
            wrap.appendChild(mt);
            return;
        }

        const tblWrap = document.createElement('div');
        tblWrap.dataset.id = 'gp-tbl-wrap';

        const tbl = document.createElement('table');
        tbl.className = 'gp-table';
        tbl.innerHTML = `
            <thead>
                <tr>
                    <th>Remote</th>
                    <th>Fetch URL</th>
                    <th>Push URL</th>
                </tr>
            </thead>
        `;

        const tbody = document.createElement('tbody');
        for (const remote of data.remotes) {
            const tr = document.createElement('tr');
            tr.innerHTML = `
                <td class="remote-name">${this._esc(remote.name)}</td>
                <td class="url-cell">${this._esc(remote.fetch)}</td>
                <td class="url-cell">${this._esc(remote.push)}</td>
            `;
            tbody.appendChild(tr);
        }
        tbl.appendChild(tbody);
        tblWrap.appendChild(tbl);
        wrap.appendChild(tblWrap);
    }

    _showError(wrap, msg) {
        const div = document.createElement('div');
        div.className = 'gp-error';
        div.dataset.id = 'gp-err';
        div.textContent = msg;
        wrap.appendChild(div);
    }

    _esc(s) {
        return String(s ?? '')
            .replace(/&/g, '&amp;')
            .replace(/</g, '&lt;')
            .replace(/>/g, '&gt;')
            .replace(/"/g, '&quot;');
    }
}

customElements.define('git-provider', GitProvider);
