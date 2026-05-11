// test-runner.mjs
// Custom element <test-runner> — lists CTest tests with Build and Run buttons
import { ApplicationState } from '../automata/applicationState.mjs';
import './components/console-output.mjs';

class TestRunner extends HTMLElement {
    async connectedCallback() {
        this.innerHTML = 'Loading tests...';
        try {
            const data = await ApplicationState.cacheFetch(ApplicationState.DASHBOARD_URL);
            this._render(data.tests || []);
        } catch (e) {
            this.innerHTML = `<span style='color:red'>Failed to load tests: ${e}</span>`;
        }
    }

    _render(tests) {
        this.innerHTML = '';
        if (!tests.length) {
            this.textContent = 'No tests found.';
            return;
        }
        const table = document.createElement('table');
        table.style.cssText = 'width:100%;border-collapse:collapse;font-size:0.9em;';

        const thead = table.createTHead();
        const hr = thead.insertRow();
        ['#', 'Test Name', 'Actions', 'Status'].forEach(h => {
            const th = document.createElement('th');
            th.textContent = h;
            th.style.cssText = 'text-align:left;padding:4px 8px;border-bottom:1px solid #ddd;';
            hr.appendChild(th);
        });

        const tbody = table.createTBody();
        tests.forEach(test => {
            const row = tbody.insertRow();
            row.style.borderBottom = '1px solid #f0f0f0';

            const numCell = row.insertCell();
            numCell.textContent = test.num;
            numCell.style.padding = '4px 8px';
            numCell.style.color = '#888';

            const nameCell = row.insertCell();
            nameCell.textContent = test.name;
            nameCell.style.padding = '4px 8px';
            nameCell.style.fontFamily = 'monospace';

            const actionsCell = row.insertCell();
            actionsCell.style.padding = '4px 8px';
            actionsCell.style.whiteSpace = 'nowrap';

            const statusCell = row.insertCell();
            statusCell.style.padding = '4px 8px';

            // Console element — one per test row, spans below
            const consoleRow = tbody.insertRow();
            const consoleCell = consoleRow.insertCell();
            consoleCell.colSpan = 4;
            consoleCell.style.padding = '0 8px 4px 8px';
            const con = document.createElement('console-output');
            consoleCell.appendChild(con);

            const mkBtn = (label, action) => {
                const btn = document.createElement('button');
                btn.textContent = label;
                btn.style.cssText = 'font-size:0.8em;padding:2px 10px;margin-right:4px;';
                btn.addEventListener('click', () => action(btn, con, statusCell));
                return btn;
            };

            actionsCell.appendChild(mkBtn('Build', async (btn, con, stat) => {
                btn.disabled = true;
                con.clear();
                con.setStatus('Building...', null);
                try {
                    const res = await fetch('/test/build', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            name: test.name,
                            working_dir: test.working_dir || '',
                            build_target: test.build_target || '',
                        }),
                    });
                    const j = await res.json();
                    con.set(j.output);
                    con.setStatus(j.success ? 'Build succeeded' : 'Build failed', j.success);
                    stat.textContent = j.success ? '✓ built' : '✗ build failed';
                    stat.style.color = j.success ? 'green' : '#e05252';
                } catch (e) {
                    con.setStatus('Error: ' + e, false);
                } finally { btn.disabled = false; }
            }));

            actionsCell.appendChild(mkBtn('Run', async (btn, con, stat) => {
                btn.disabled = true;
                con.clear();
                con.setStatus('Running...', null);
                try {
                    const res = await fetch('/test/run', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ name: test.name }),
                    });
                    const j = await res.json();
                    con.set(j.output);
                    con.setStatus(j.success ? 'Passed' : 'Failed', j.success);
                    stat.textContent = j.success ? '✓ passed' : '✗ failed';
                    stat.style.color = j.success ? 'green' : '#e05252';
                } catch (e) {
                    con.setStatus('Error: ' + e, false);
                } finally { btn.disabled = false; }
            }));
        });

        this.appendChild(table);
    }
}

customElements.define('test-runner', TestRunner);
