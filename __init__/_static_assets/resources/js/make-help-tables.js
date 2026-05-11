import './make-help-table.js';

class MakeHelpTables extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({mode: 'open'});
    this.sections = [
      { key: 'apps', title: 'Apps' },
      { key: 'demos', title: 'Demos' },
      { key: 'services', title: 'Services' },
      { key: 'libraries', title: 'Libraries' }
    ];
    this.data = {};
  }
  set tablesData(obj) {
    this.data = obj || {};
    this.render();
  }
  render() {
    this.shadowRoot.innerHTML = `
      <style>
        :host { display: block; }
      </style>
      ${this.sections.map(sec => `
        <make-help-table section="${sec.key}" title="${sec.title}"></make-help-table>
      `).join('')}
    `;
    this.sections.forEach(sec => {
      const el = this.shadowRoot.querySelector(`make-help-table[section="${sec.key}"]`);
      if (el) el.tableData = this.data[sec.key] || [];
    });
  }
}
customElements.define('make-help-tables', MakeHelpTables);
