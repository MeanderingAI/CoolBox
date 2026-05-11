/**
 * <code-shower src="...">
 * Displays a source/header file with syntax highlighting, line numbers, and a copy button.
 * Set the `src` attribute to a repo-relative path; the element fetches /library/file?path=<src>.
 */

const KEYWORDS = new Set([
    'if','else','for','while','do','return','switch','case','break','continue',
    'const','static','void','struct','class','namespace','using','template',
    'typename','public','private','protected','virtual','override','inline',
    'explicit','noexcept','nullptr','true','false','auto','new','delete','this',
    'operator','friend','enum','typedef','extern','volatile','mutable','constexpr',
    'decltype','static_assert','sizeof','alignof','try','catch','throw','default',
]);
const TYPES = new Set([
    'int','long','short','char','bool','float','double','unsigned','signed',
    'uint8_t','uint16_t','uint32_t','uint64_t','int8_t','int16_t','int32_t','int64_t',
    'size_t','ptrdiff_t','string','vector','map','unordered_map','set','pair',
    'shared_ptr','unique_ptr','weak_ptr','optional','variant','span','array','tuple',
    'wchar_t','char16_t','char32_t','char8_t',
]);
const CODE_EXTS = new Set(['h','hpp','hxx','c','cpp','cxx','cc','inl']);

const STYLE = `
:host {
    display: flex;
    flex-direction: column;
    height: 100%;
    min-height: 0;
    background: #1e1e1e;
    border-radius: 0 6px 6px 0;
    overflow: hidden;
    font-family: 'Cascadia Code', 'Fira Code', 'Consolas', monospace;
}

/* ── Header bar ─────────────────────────────────────── */
.cs-header {
    display: flex;
    align-items: center;
    gap: 0.5em;
    padding: 0.4em 0.8em;
    background: #2d2d30;
    border-bottom: 1px solid #3e3e42;
    flex-shrink: 0;
    min-height: 32px;
}
.cs-filename {
    color: #9cdcfe;
    font-size: 0.82em;
    flex: 1;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.cs-copy {
    background: none;
    border: 1px solid #555;
    border-radius: 3px;
    color: #ccc;
    cursor: pointer;
    font-size: 0.72em;
    padding: 2px 8px;
    flex-shrink: 0;
    font-family: inherit;
    transition: background 0.12s, color 0.12s;
}
.cs-copy:hover { background: #3e3e42; color: #fff; }

/* ── Body / scroll area ──────────────────────────────── */
.cs-body {
    flex: 1;
    overflow: auto;
    min-height: 0;
}
.cs-empty {
    display: flex;
    align-items: center;
    justify-content: center;
    height: 100%;
    color: #555;
    font-size: 0.85em;
    font-family: 'Segoe UI', sans-serif;
    user-select: none;
}
.cs-empty.error { color: #f48771; }

/* ── Code table ──────────────────────────────────────── */
table {
    border-collapse: collapse;
    width: 100%;
    min-width: max-content;
}
td { padding: 0; vertical-align: top; }

.gutter {
    color: #858585;
    font-size: 0.79em;
    padding: 0 0.7em;
    text-align: right;
    user-select: none;
    border-right: 1px solid #2d2d30;
    min-width: 3em;
    white-space: nowrap;
    background: #1e1e1e;
    line-height: 1.5;
}
.code-line {
    color: #d4d4d4;
    font-size: 0.8em;
    padding: 0 1em 0 0.8em;
    white-space: pre;
    tab-size: 4;
    line-height: 1.5;
}
tr:hover .gutter  { color: #bbb; }
tr:hover .code-line { background: rgba(255,255,255,0.035); }

/* ── Syntax colours (VS Code Dark+) ─────────────────── */
.kw { color: #569cd6; }        /* keywords   */
.ty { color: #4ec9b0; }        /* types      */
.cm { color: #6a9955; }        /* comments   */
.st { color: #ce9178; }        /* strings    */
.pp { color: #c586c0; }        /* #preprocessor */
.nm { color: #b5cea8; }        /* numbers    */
`;

class CodeShower extends HTMLElement {
    static get observedAttributes() { return ['src']; }

    connectedCallback() {
        if (!this.shadowRoot) this._build();
    }

    attributeChangedCallback(name, _old, newVal) {
        if (name === 'src' && this.shadowRoot) this._load(newVal || '');
    }

    _build() {
        const shadow = this.attachShadow({ mode: 'open' });
        const style = document.createElement('style');
        style.textContent = STYLE;
        shadow.appendChild(style);

        const header = document.createElement('div');
        header.className = 'cs-header';

        this._filename = document.createElement('span');
        this._filename.className = 'cs-filename';
        this._filename.textContent = 'No file selected';
        header.appendChild(this._filename);

        this._copyBtn = document.createElement('button');
        this._copyBtn.className = 'cs-copy';
        this._copyBtn.textContent = 'Copy';
        header.appendChild(this._copyBtn);

        this._body = document.createElement('div');
        this._body.className = 'cs-body';
        this._showEmpty('Select a header file to view');

        shadow.appendChild(header);
        shadow.appendChild(this._body);

        this._content = '';
        this._copyBtn.addEventListener('click', () => {
            if (!this._content) return;
            navigator.clipboard.writeText(this._content).then(() => {
                this._copyBtn.textContent = 'Copied!';
                setTimeout(() => { this._copyBtn.textContent = 'Copy'; }, 1500);
            });
        });

        const src = this.getAttribute('src');
        if (src) this._load(src);
    }

    _showEmpty(msg, isError = false) {
        const el = document.createElement('div');
        el.className = 'cs-empty' + (isError ? ' error' : '');
        el.textContent = msg;
        this._body.innerHTML = '';
        this._body.appendChild(el);
    }

    async _load(src) {
        if (!src) { this._showEmpty('Select a header file to view'); return; }
        this._filename.textContent = src.split('/').pop();
        this._showEmpty('Loading…');
        try {
            const r = await fetch(`/library/file?path=${encodeURIComponent(src)}`);
            const data = await r.json();
            if (data.error) throw new Error(data.error);
            this._content = data.content;
            this._renderCode(data.content, data.name);
        } catch (e) {
            this._showEmpty(`Error: ${e.message}`, true);
        }
    }

    _renderCode(text, filename) {
        const ext = (filename || '').split('.').pop().toLowerCase();
        const highlight = CODE_EXTS.has(ext);
        const lines = text.split('\n');
        // Remove single trailing empty line that's an artefact of the final \n
        if (lines.length > 1 && lines[lines.length - 1] === '') lines.pop();

        const table = document.createElement('table');
        let inBC = false;
        lines.forEach((line, idx) => {
            const tr = document.createElement('tr');

            const gutter = document.createElement('td');
            gutter.className = 'gutter';
            gutter.textContent = idx + 1;

            const code = document.createElement('td');
            code.className = 'code-line';
            if (highlight) {
                const { html, blockComment } = this._tokenizeLine(line, inBC);
                inBC = blockComment;
                code.innerHTML = html;
            } else {
                code.textContent = line;
            }

            tr.appendChild(gutter);
            tr.appendChild(code);
            table.appendChild(tr);
        });

        this._body.innerHTML = '';
        this._body.appendChild(table);
    }

    // ── Tokenizer ──────────────────────────────────────────
    _esc(s) {
        return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }

    /**
     * Tokenise a single line, returning {html, blockComment: bool}.
     * blockComment indicates whether the line ends while still inside a /* ... * / block.
     */
    _tokenizeLine(line, inBlockComment) {
        let html = '';
        let i = 0;

        if (inBlockComment) {
            const end = line.indexOf('*/');
            if (end === -1) {
                return { html: `<span class="cm">${this._esc(line)}</span>`, blockComment: true };
            }
            html += `<span class="cm">${this._esc(line.slice(0, end + 2))}</span>`;
            i = end + 2;
            if (i >= line.length) return { html, blockComment: false };
        }

        // Check for preprocessor line (ignoring leading whitespace)
        const trimmed = line.slice(i).trimStart();
        if (trimmed.startsWith('#')) {
            html += `<span class="pp">${this._esc(line.slice(i))}</span>`;
            return { html, blockComment: false };
        }

        // Token loop
        while (i < line.length) {
            const ch = line[i];
            const next = line[i + 1];

            // Block comment start
            if (ch === '/' && next === '*') {
                const end = line.indexOf('*/', i + 2);
                if (end === -1) {
                    html += `<span class="cm">${this._esc(line.slice(i))}</span>`;
                    return { html, blockComment: true };
                }
                html += `<span class="cm">${this._esc(line.slice(i, end + 2))}</span>`;
                i = end + 2;
                continue;
            }

            // Line comment
            if (ch === '/' && next === '/') {
                html += `<span class="cm">${this._esc(line.slice(i))}</span>`;
                return { html, blockComment: false };
            }

            // String literal
            if (ch === '"') {
                let j = i + 1;
                while (j < line.length && line[j] !== '"') {
                    if (line[j] === '\\') j++;
                    j++;
                }
                if (j < line.length) j++; // closing "
                html += `<span class="st">${this._esc(line.slice(i, j))}</span>`;
                i = j;
                continue;
            }

            // Char literal
            if (ch === "'") {
                let j = i + 1;
                while (j < line.length && line[j] !== "'") {
                    if (line[j] === '\\') j++;
                    j++;
                }
                if (j < line.length) j++;
                html += `<span class="st">${this._esc(line.slice(i, j))}</span>`;
                i = j;
                continue;
            }

            // Number
            if (/[0-9]/.test(ch)) {
                let j = i;
                if (line[j] === '0' && /[xXbBoO]/.test(line[j + 1] || '')) {
                    j += 2;
                    while (j < line.length && /[0-9a-fA-F_]/.test(line[j])) j++;
                } else {
                    while (j < line.length && /[0-9\._]/.test(line[j])) j++;
                    while (j < line.length && /[uUlLfF]/.test(line[j])) j++;
                }
                html += `<span class="nm">${this._esc(line.slice(i, j))}</span>`;
                i = j;
                continue;
            }

            // Identifier / keyword / type
            if (/[a-zA-Z_]/.test(ch)) {
                let j = i + 1;
                while (j < line.length && /[a-zA-Z0-9_]/.test(line[j])) j++;
                const word = line.slice(i, j);
                if (KEYWORDS.has(word)) {
                    html += `<span class="kw">${this._esc(word)}</span>`;
                } else if (TYPES.has(word)) {
                    html += `<span class="ty">${this._esc(word)}</span>`;
                } else {
                    html += this._esc(word);
                }
                i = j;
                continue;
            }

            html += this._esc(ch);
            i++;
        }

        return { html, blockComment: false };
    }
}

customElements.define('code-shower', CodeShower);
