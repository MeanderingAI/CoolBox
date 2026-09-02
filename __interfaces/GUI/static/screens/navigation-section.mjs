// Custom element <navigation-section>
// Renders a tab bar with Package Builder and Git tabs.
// Tests have moved inside Package Builder as a sub-tab.
// Each tab panel lazily mounts its child custom element on first activation.
import './package_builder/package-builder.mjs';
import './git_history/git-section.mjs';
import './network-info.mjs';
import './client-portals.mjs';

const TABS = [
    { id: 'libraries', label: '📦 Package Builder', tag: 'package-builder' },
    { id: 'git',       label: '🔀 Git',              tag: 'git-section'     },
    { id: 'network',   label: '🌐 Network',           tag: 'network-info'    },
    { id: 'portals',   label: '🔗 Client Portals',    tag: 'client-portals'  },
];

const STYLE = `
:host { display: block; }

/* ── Wrapper card ─────────────────────────────────────── */
.nav-card {
    background: #fff;
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
    overflow: hidden;
}

/* ── Tab bar ──────────────────────────────────────────── */
.tab-bar {
    display: flex;
    flex-wrap: wrap;
    background: #1e1e2e;
    border-bottom: 2px solid #0e639c;
    gap: 0;
}

.tab-btn {
    flex: 1 1 auto;
    min-width: 140px;
    padding: 0.65em 1.4em;
    font-size: 0.9em;
    font-family: inherit;
    background: transparent;
    border: none;
    border-bottom: 3px solid transparent;
    margin-bottom: -2px;
    cursor: pointer;
    color: #9ca3af;
    transition: color 0.15s, background 0.15s, border-color 0.15s;
    white-space: nowrap;
}

.tab-btn:hover {
    background: #2a2a3e;
    color: #e5e7eb;
}

.tab-btn[aria-selected="true"] {
    background: #16213e;
    color: #fff;
    border-bottom-color: #0e639c;
    font-weight: 600;
}

/* ── Panel ────────────────────────────────────────────── */
.tab-panel { display: none; }
.tab-panel.active {
    display: block;
    padding: 1em 1.5em;
}


`;

class NavigationSection extends HTMLElement {
    connectedCallback() {
        const shadow = this.attachShadow({ mode: 'open' });

        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        // Wrapper card
        const card = document.createElement('div');
        card.className = 'nav-card';
        shadow.appendChild(card);

        // Tab bar
        const bar = document.createElement('div');
        bar.className = 'tab-bar';
        bar.setAttribute('role', 'tablist');
        card.appendChild(bar);

        // Panels
        const panels = TABS.map(tab => {
            const btn = document.createElement('button');
            btn.className = 'tab-btn';
            btn.textContent = tab.label;
            btn.setAttribute('role', 'tab');
            btn.setAttribute('aria-selected', 'false');
            btn.setAttribute('aria-controls', `panel-${tab.id}`);
            btn.dataset.tab = tab.id;
            bar.appendChild(btn);

            const panel = document.createElement('div');
            panel.className = 'tab-panel';
            panel.id = `panel-${tab.id}`;
            panel.setAttribute('role', 'tabpanel');
            panel.dataset.tag = tab.tag;
            panel.dataset.mounted = 'false';
            card.appendChild(panel);

            btn.addEventListener('click', () => this._activate(tab.id, shadow, bar));
            return panel;
        });

        // Activate first tab immediately
        this._activate(TABS[0].id, shadow, bar);

        // Cross-component navigation: listen for coolbox:navigate dispatched on document
        document.addEventListener('coolbox:navigate', (e) => {
            const { tab, subtab } = e.detail || {};
            if (tab) this._activate(tab, shadow, bar);
            if (subtab) {
                // Give the newly mounted custom element a tick to call connectedCallback
                setTimeout(() => {
                    document.dispatchEvent(new CustomEvent('coolbox:subtab', {
                        detail: { subtab }
                    }));
                }, 60);
            }
        });
    }

    _activate(tabId, shadow, bar) {
        // Update buttons
        bar.querySelectorAll('.tab-btn').forEach(btn => {
            btn.setAttribute('aria-selected', btn.dataset.tab === tabId ? 'true' : 'false');
        });

        // Update panels — lazily mount the custom element on first show
        shadow.querySelectorAll('.tab-panel').forEach(panel => {
            const isActive = panel.id === `panel-${tabId}`;
            panel.classList.toggle('active', isActive);
            if (isActive && panel.dataset.mounted === 'false') {
                panel.dataset.mounted = 'true';
                const el = document.createElement(panel.dataset.tag);
                panel.appendChild(el);
            }
        });
    }
}

customElements.define('navigation-section', NavigationSection);
