// Auto-generated from make-help-table.js
#pragma once

namespace resources {
    constexpr const char* make_help_table_js = R"JS(
class MakeHelpTable extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({mode: 'open'});
    this.section = this.getAttribute('section') || '';
    this.title = this.getAttribute('title') || '';
    this.data = [];
  }
  set tableData(arr) {
    this.data = arr || [];
    this.render();
  }
  render() {
    this.shadowRoot.innerHTML = `
      <style>
        .swipe-table-container { overflow-x: auto; margin-bottom: 24px; background: rgba(255,255,255,0.02); border-radius: 8px; box-shadow: 0 1px 4px rgba(139,92,246,0.08); padding: 8px 0; }
        h2 { color: #a78bfa; font-size:1.1em; margin:8px 0 4px 8px; }
        table { min-width: 300px; width: 100%; border-collapse: collapse; background: transparent; }
        th, td { padding: 12px 16px; text-align: left; }
        th { background: #2d2d44; color: #a78bfa; font-size: 0.95rem; font-weight: 700; border-bottom: 2px solid #8b5cf6; }
        tr { border-bottom: 1px solid #232336; }
        tr:last-child { border-bottom: none; }
        td { font-size: 0.97rem; }
      </style>
      <div class="swipe-table-container">
        <h2>${this.title}</h2>
        <table class="swipe-table">
          <thead><tr><th>Name</th></tr></thead>
          <tbody>
            ${this.data.length ? this.data.map(x => `<tr><td>${x.replace(/\s*Executable:|Library:/,'').trim()}</td></tr>`).join('') : '<tr><td>No entries</td></tr>'}
          </tbody>
        </table>
      </div>
    `;
  }
}
customElements.define('make-help-table', MakeHelpTable);

)JS";
}
