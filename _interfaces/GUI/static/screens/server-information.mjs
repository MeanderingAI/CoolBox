// server-information.mjs
// Custom element <server-information> for rendering server meta info
import { ApplicationState } from '../automata/applicationState.mjs';

class ServerInformation extends HTMLElement {
    async connectedCallback() {
        this.innerHTML = 'Loading server info...';
        try {
            const data = await ApplicationState.cacheFetch(ApplicationState.DASHBOARD_URL);
            const info = [
                { label: 'Server URL', value: data.server_url },
                { label: 'User', value: data.user },
                { label: 'Branch', value: data.branch },
            ];
            this.style.cssText = 'display:flex;gap:1.5em;font-size:0.85em;align-items:center;';
            this.innerHTML = '';
            info.forEach(item => {
                const span = document.createElement('span');
                span.innerHTML = `<strong>${item.label}:</strong> ${item.value}`;
                this.appendChild(span);
            });
        } catch (e) {
            this.innerHTML = `<span style='color:red'>Failed to load server info: ${e}</span>`;
        }
    }
}

customElements.define('server-information', ServerInformation);
