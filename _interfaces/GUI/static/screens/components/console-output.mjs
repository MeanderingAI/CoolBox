// console-output.mjs
// Generic <console-output> custom element for displaying command output
// Usage: <console-output></console-output>
// API:  element.write(text)  — append text
//       element.clear()      — clear output
//       element.setStatus(msg, ok) — set a status line

class ConsoleOutput extends HTMLElement {
    connectedCallback() {
        this.style.display = 'block';
        this._status = document.createElement('div');
        this._status.style.cssText = 'font-size:0.8em;margin-bottom:0.3em;';
        this._pre = document.createElement('pre');
        this._pre.style.cssText = [
            'margin:0',
            'font-size:0.78em',
            'background:#1e1e1e',
            'color:#d4d4d4',
            'padding:0.6em 0.8em',
            'border-radius:4px',
            'max-height:260px',
            'overflow:auto',
            'white-space:pre-wrap',
            'word-break:break-all',
            'display:none',
        ].join(';');
        this.appendChild(this._status);
        this.appendChild(this._pre);
    }

    /** Append text to the console and show it. */
    write(text) {
        this._pre.style.display = 'block';
        this._pre.textContent += text;
        this._pre.scrollTop = this._pre.scrollHeight;
    }

    /** Replace all console content with text. */
    set(text) {
        this._pre.style.display = text ? 'block' : 'none';
        this._pre.textContent = text || '';
    }

    /** Clear the console. */
    clear() {
        this._pre.textContent = '';
        this._pre.style.display = 'none';
    }

    /** Set the status line. ok=true → green, ok=false → red, ok=null → grey */
    setStatus(msg, ok = null) {
        this._status.textContent = msg;
        this._status.style.color = ok === true ? 'green' : ok === false ? '#e05252' : '#888';
    }
}

customElements.define('console-output', ConsoleOutput);
