// Custom element <git-pipeline>
// Pipeline runner and YAML preview for Git tab

const STYLE = `
:host { display: block; font-family: inherit; }
.pipeline-controls {
  margin-bottom: 1em;
}
.pipeline-btn {
  margin-right: 1em;
  padding: 0.5em 1.2em;
  font-size: 1em;
  border-radius: 4px;
  border: 1px solid #0e639c;
  background: #0e639c;
  color: #fff;
  cursor: pointer;
}
.pipeline-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}
.pipeline-output {
  background: #222;
  color: #c3e88d;
  font-family: monospace;
  padding: 1em;
  border-radius: 6px;
  min-height: 180px;
  white-space: pre-wrap;
  margin-top: 1em;
}
`;

class GitPipeline extends HTMLElement {
  async connectedCallback() {
    const shadow = this.attachShadow({ mode: 'open' });
    const style = document.createElement('style');
    style.textContent = STYLE;
    shadow.appendChild(style);

    const dockerBox = document.createElement('div');
    dockerBox.style.marginBottom = '0.9em';
    dockerBox.innerHTML = `
      <label style="font-size:0.95em;color:#0e639c;">Docker:</label>
      <span id="docker-status" style="margin-left:0.5em;color:#e5c07b;font-size:0.95em;">Checking...</span>
      <button id="docker-start" class="pipeline-btn" style="padding:0.35em 0.9em;margin-left:0.8em;">Start Docker</button>
    `;
    shadow.appendChild(dockerBox);

    // Workflow controls
    const wfBox = document.createElement('div');
    wfBox.style.marginBottom = '1em';
    wfBox.innerHTML = `
      <label style="font-size:0.95em;color:#0e639c;">GitHub Workflow:</label>
      <select id="workflow-select" style="min-width:260px;padding:0.4em;margin:0 0.5em 0 0.5em;"></select>
      <button id="act-run" class="pipeline-btn" style="padding:0.4em 1em;">⚡ Run with act</button>
      <span id="act-status" style="margin-left:1em;color:#43d17a;font-size:0.95em;"></span>
      <div id="workflow-yaml-preview" style="margin-top:0.6em;color:#9cdcfe;font-size:0.92em;"></div>
    `;
    shadow.appendChild(wfBox);

    const output = document.createElement('pre');
    output.className = 'pipeline-output';
    output.textContent = 'Run output will appear here.';
    shadow.appendChild(output);

    const dockerStatus = dockerBox.querySelector('#docker-status');
    const dockerStart = dockerBox.querySelector('#docker-start');
    const workflowSelect = wfBox.querySelector('#workflow-select');
    const actRun = wfBox.querySelector('#act-run');
    const actStatus = wfBox.querySelector('#act-status');
    const workflowYamlPreview = wfBox.querySelector('#workflow-yaml-preview');

    const refreshDockerStatus = async () => {
      dockerStatus.style.color = '#e5c07b';
      dockerStatus.textContent = 'Checking...';
      try {
        const resp = await fetch('/api/pipeline/docker-status');
        const data = await resp.json();
        if (data.running) {
          dockerStatus.style.color = '#43d17a';
          dockerStatus.textContent = 'Running';
          dockerStart.disabled = true;
        } else {
          dockerStatus.style.color = '#ff6b6b';
          dockerStatus.textContent = 'Not running';
          dockerStart.disabled = false;
        }
      } catch {
        dockerStatus.style.color = '#ff6b6b';
        dockerStatus.textContent = 'Status unavailable';
        dockerStart.disabled = false;
      }
    };

    dockerStart.addEventListener('click', async () => {
      dockerStart.disabled = true;
      dockerStatus.style.color = '#e5c07b';
      dockerStatus.textContent = 'Starting...';
      output.textContent = 'Calling /api/pipeline/docker-start...';
      try {
        const resp = await fetch('/api/pipeline/docker-start', { method: 'POST' });
        const data = await resp.json();
        if (data.running) {
          dockerStatus.style.color = '#43d17a';
          dockerStatus.textContent = 'Running';
          output.textContent = 'Docker is running.';
          dockerStart.disabled = true;
        } else {
          dockerStatus.style.color = '#ff6b6b';
          dockerStatus.textContent = 'Not running';
          output.textContent = 'Tried to start Docker, but it is still not available.';
          dockerStart.disabled = false;
        }
      } catch (err) {
        dockerStatus.style.color = '#ff6b6b';
        dockerStatus.textContent = 'Start failed';
        output.textContent = `Failed to start Docker: ${err instanceof Error ? err.message : String(err)}`;
        dockerStart.disabled = false;
      }
    });

    const renderWorkflowYamlPreview = () => {
      const selected = workflowSelect.value;
      if (!selected) {
        workflowYamlPreview.textContent = 'Workflow YAML to run: (none selected)';
        return;
      }
      workflowYamlPreview.textContent = `Workflow YAML to run: .github/workflows/${selected}`;
    };

    actRun.addEventListener('click', async () => {
      const selected = workflowSelect.value;
      if (!selected) {
        actStatus.style.color = '#ff6b6b';
        actStatus.textContent = 'Select a workflow first';
        output.textContent = 'No workflow selected.';
        return;
      }

      actRun.disabled = true;
      actStatus.style.color = '#e5c07b';
      actStatus.textContent = 'Running...';
      output.textContent = `Calling /api/pipeline/act for ${selected}...`;

      try {
        const resp = await fetch('/api/pipeline/act', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ workflow: selected, event: 'push' })
        });

        const text = await resp.text();
        output.textContent = text || '(no output)';

        if (resp.ok) {
          actStatus.style.color = '#43d17a';
          actStatus.textContent = 'Completed';
        } else {
          actStatus.style.color = '#ff6b6b';
          actStatus.textContent = `Failed (${resp.status})`;
        }
      } catch (err) {
        actStatus.style.color = '#ff6b6b';
        actStatus.textContent = 'Request error';
        output.textContent = `Request failed: ${err instanceof Error ? err.message : String(err)}`;
      } finally {
        actRun.disabled = false;
      }
    });

    workflowSelect.addEventListener('change', renderWorkflowYamlPreview);

    // Load workflows
    try {
      const resp = await fetch('/api/pipeline/workflows');
      const data = await resp.json();
      for (const wf of data.workflows) {
        const opt = document.createElement('option');
        opt.value = wf;
        opt.textContent = wf;
        workflowSelect.appendChild(opt);
      }
      renderWorkflowYamlPreview();
    } catch {}

    await refreshDockerStatus();
    renderWorkflowYamlPreview();


    // (removed duplicate block)
  }
}
customElements.define('git-pipeline', GitPipeline);
