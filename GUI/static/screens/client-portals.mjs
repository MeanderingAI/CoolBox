// Custom element <client-portals>
// Placeholder panel for future client portal links.

const STYLE = `
:host { display: block; font-family: inherit; }

.cp-body {
    padding: 2em 1.5em;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 0.6em;
    color: #9ca3af;
    text-align: center;
}

.cp-icon {
    font-size: 2.4em;
    line-height: 1;
}

.cp-title {
    font-size: 0.95em;
    font-weight: 600;
    color: #6b7280;
}

.cp-sub {
    font-size: 0.82em;
    font-style: italic;
    max-width: 340px;
    line-height: 1.5;
}
`;

class ClientPortals extends HTMLElement {
    connectedCallback() {
        if (this._xLoaded) return;
        this._xLoaded = true;

        const shadow = this.attachShadow({ mode: 'open' });
        shadow.innerHTML = `
            <style>${STYLE}</style>
            <div class="cp-body">
                <div class="cp-icon">🔗</div>
                <div class="cp-title">No client portals configured</div>
                <div class="cp-sub">Portal links and external service shortcuts will appear here once configured.</div>
            </div>
        `;
    }
}

customElements.define('client-portals', ClientPortals);
