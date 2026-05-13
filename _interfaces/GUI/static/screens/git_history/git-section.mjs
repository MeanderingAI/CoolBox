// Custom element <git-section>
// Container with sub-tabs: History | Diff | Stack | Commit | Remotes | Workspace | Plans.
import './git-history.mjs';
import './git-diff.mjs';
import './git-stack.mjs';
import './git-commit.mjs';
import './git-merge.mjs';
import './git-provider.mjs';
import '../workspace-editor.mjs';
import './plan-canvas.mjs';
import './git-sub-repos.mjs';
import './git-experiment-center.mjs';

const SUB_TABS = [
    { id: 'history',    label: '📋 History',          tag: 'git-history'           },
    { id: 'diff',       label: '🔍 Diff',              tag: 'git-diff'              },
    { id: 'stack',      label: '📦 Stack',             tag: 'git-stack'             },
    { id: 'commit',     label: '✏️ Commit',            tag: 'git-commit'            },
    { id: 'merge',      label: '🔀 Merge',             tag: 'git-merge'             },
    { id: 'remotes',    label: '🌐 Remotes',           tag: 'git-provider'          },
    { id: 'workspace',  label: '✏️ Workspace',         tag: 'workspace-editor'      },
    { id: 'plans',      label: '🗺 Plans',             tag: 'plan-canvas'           },
    { id: 'subrepos',   label: '📂 Sub-Repos',         tag: 'git-sub-repos'         },
    { id: 'experiment', label: '🧪 Experiment Centre', tag: 'git-experiment-center' },
];

const STYLE = `
:host { display: block; font-family: inherit; }

/* ── Sub-tab bar ─────────────────────────────────────── */
.sub-bar {
    display: flex;
    gap: 0.4em;
    padding: 0.75em 1em 0;
    background: #f5f6fa;
    border-bottom: 1px solid #dde1ea;
    flex-wrap: wrap;
}

.sub-btn {
    padding: 0.35em 1.1em;
    font-size: 0.82em;
    font-family: inherit;
    background: #fff;
    border: 1px solid #c5cad8;
    border-radius: 5px 5px 0 0;
    border-bottom: none;
    cursor: pointer;
    color: #555;
    margin-bottom: -1px;
    transition: background 0.12s, color 0.12s;
    white-space: nowrap;
}
.sub-btn:hover { background: #e8eaf0; color: #222; }
.sub-btn[aria-selected="true"] {
    background: #fff;
    border-color: #0e639c;
    color: #0e639c;
    font-weight: 600;
    border-bottom: 1px solid #fff;
}

/* ── Panels ──────────────────────────────────────────── */
.sub-panel { display: none; padding: 1em; }
.sub-panel.active { display: block; }

/* Workspace editor needs full height and no padding */
#sub-panel-workspace.active {
    display: flex;
    flex-direction: column;
    padding: 0;
    height: calc(100vh - 175px);
    overflow: hidden;
}

/* Plan canvas also needs full height and no padding */
#sub-panel-plans.active {
    display: flex;
    flex-direction: column;
    padding: 0;
    height: calc(100vh - 175px);
    overflow: hidden;
}

/* Experiment Centre: full height, no padding, dark bg */
#sub-panel-experiment.active {
    display: flex;
    flex-direction: column;
    padding: 0;
    height: calc(100vh - 175px);
    overflow: hidden;
    background: #1e1e2e;
}
`;

class GitSection extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        const bar = document.createElement('div');
        bar.className = 'sub-bar';
        bar.setAttribute('role', 'tablist');
        shadow.appendChild(bar);

        const panels = SUB_TABS.map((tab, i) => {
            const btn = document.createElement('button');
            btn.className = 'sub-btn';
            btn.textContent = tab.label;
            btn.setAttribute('role', 'tab');
            btn.setAttribute('aria-selected', i === 0 ? 'true' : 'false');
            btn.setAttribute('aria-controls', `sub-panel-${tab.id}`);
            bar.appendChild(btn);

            const panel = document.createElement('div');
            panel.className = 'sub-panel' + (i === 0 ? ' active' : '');
            panel.id = `sub-panel-${tab.id}`;
            panel.setAttribute('role', 'tabpanel');
            panel.dataset.tag = tab.tag;
            panel.dataset.mounted = 'false';
            shadow.appendChild(panel);

            btn.addEventListener('click', () => this._activate(tab.id, shadow, bar));
            return panel;
        });

        // Mount the first tab immediately
        this._mount(panels[0]);
    }

    _activate(tabId, shadow, bar) {
        SUB_TABS.forEach(tab => {
            const btn = bar.querySelector(`[aria-controls="sub-panel-${tab.id}"]`);
            const panel = shadow.getElementById(`sub-panel-${tab.id}`);
            const active = tab.id === tabId;
            btn?.setAttribute('aria-selected', active ? 'true' : 'false');
            panel?.classList.toggle('active', active);
            if (active && panel) this._mount(panel);
        });
    }

    _mount(panel) {
        if (panel.dataset.mounted === 'true') return;
        panel.dataset.mounted = 'true';
        panel.appendChild(document.createElement(panel.dataset.tag));
    }
}

customElements.define('git-section', GitSection);
