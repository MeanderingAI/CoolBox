// Custom element <groups-libraries> for rendering groups and libraries with checkboxes and build buttons
import { ApplicationState } from '../../automata/applicationState.mjs';

class GroupsLibraries extends HTMLElement {
    async connectedCallback() {
        this.innerHTML = '<div>Loading groups...</div>';
        try {
            const data = await ApplicationState.cacheFetch(ApplicationState.DASHBOARD_URL);
            this.renderGroups(data.groups);
        } catch (e) {
            this.innerHTML = `<div style="color:red">Failed to load groups: ${e}</div>`;
        }
    }

    renderGroups(groups) {
        this.innerHTML = '';
        groups.forEach(group => {
            const groupDiv = document.createElement('div');
            groupDiv.className = 'group';
            groupDiv.style.marginBottom = '1em';

            const groupLabel = document.createElement('strong');
            groupLabel.textContent = group.name;
            groupDiv.appendChild(groupLabel);

            const libsDiv = document.createElement('div');
            libsDiv.style.marginLeft = '1em';
            libsDiv.style.marginTop = '0.3em';

            group.libs.forEach(lib => {
                const row = document.createElement('div');
                row.style.display = 'flex';
                row.style.alignItems = 'center';
                row.style.gap = '0.7em';
                row.style.marginBottom = '0.2em';

                const label = document.createElement('label');
                label.style.minWidth = '180px';
                const checkbox = document.createElement('input');
                checkbox.type = 'checkbox';
                checkbox.name = `lib-${group.name}`;
                checkbox.value = lib;
                label.appendChild(checkbox);
                label.appendChild(document.createTextNode(' ' + lib));

                const buildBtn = document.createElement('button');
                buildBtn.textContent = 'Build';
                buildBtn.style.fontSize = '0.8em';
                buildBtn.style.padding = '2px 10px';
                buildBtn.title = `Build shared library for ${group.name}/${lib}`;

                const depsBtn = document.createElement('button');
                depsBtn.textContent = 'Deps';
                depsBtn.style.fontSize = '0.8em';
                depsBtn.style.padding = '2px 10px';
                depsBtn.title = `Show dependency graph for ${lib}`;

                const statusSpan = document.createElement('span');
                statusSpan.style.fontSize = '0.8em';

                buildBtn.addEventListener('click', async () => {
                    buildBtn.disabled = true;
                    statusSpan.textContent = 'Building...';
                    statusSpan.style.color = '#888';
                    try {
                        const resp = await fetch('/build', {
                            method: 'POST',
                            headers: { 'Content-Type': 'application/json' },
                            body: JSON.stringify({ group: group.name, lib }),
                        });
                        const result = await resp.json();
                        if (result.success) {
                            statusSpan.textContent = 'Build succeeded';
                            statusSpan.style.color = 'green';
                        } else {
                            statusSpan.textContent = 'Build failed';
                            statusSpan.style.color = 'red';
                        }
                        // Show output in a collapsible pre block
                        let pre = row.querySelector('pre.build-output');
                        if (!pre) {
                            pre = document.createElement('pre');
                            pre.className = 'build-output';
                            pre.style.cssText = 'margin:0.3em 0 0 0;font-size:0.75em;background:#1e1e1e;color:#d4d4d4;padding:0.5em;border-radius:4px;max-height:200px;overflow:auto;white-space:pre-wrap;';
                            row.appendChild(pre);
                        }
                        pre.textContent = result.output || '';
                        // Show download links for built artifacts
                        const existing = row.querySelector('.artifact-links');
                        if (existing) existing.remove();
                        if (result.success && result.artifacts && result.artifacts.length) {
                            const linksDiv = document.createElement('div');
                            linksDiv.className = 'artifact-links';
                            linksDiv.style.cssText = 'margin:0.3em 0 0 0;display:flex;flex-wrap:wrap;gap:0.4em;';
                            result.artifacts.forEach(relPath => {
                                const fname = relPath.split('/').pop();
                                const a = document.createElement('a');
                                a.href = `/download?path=${encodeURIComponent(relPath)}`;
                                a.download = fname;
                                a.textContent = `⬇ ${fname}`;
                                a.style.cssText = 'font-size:0.75em;padding:2px 8px;background:#0e639c;color:#fff;border-radius:3px;text-decoration:none;';
                                linksDiv.appendChild(a);
                            });
                            row.appendChild(linksDiv);
                        }
                    } catch (err) {
                        statusSpan.textContent = 'Error: ' + err;
                        statusSpan.style.color = 'red';
                    } finally {
                        buildBtn.disabled = false;
                    }
                });

                depsBtn.addEventListener('click', async () => {
                    depsBtn.disabled = true;
                    statusSpan.textContent = 'Fetching deps...';
                    statusSpan.style.color = '#888';
                    let depsContainer = row.parentElement.querySelector(`.deps-graph[data-lib="${lib}"]`);
                    if (depsContainer) {
                        depsContainer.remove();
                        if (statusSpan.textContent === 'Fetching deps...') statusSpan.textContent = '';
                        depsBtn.disabled = false;
                        return;
                    }
                    try {
                        const resp = await fetch(`/deps/${encodeURIComponent(lib)}`);
                        const result = await resp.json();
                        depsContainer = document.createElement('div');
                        depsContainer.className = 'deps-graph';
                        depsContainer.dataset.lib = lib;
                        depsContainer.style.cssText = 'margin:0.3em 0 0 1em;font-size:0.8em;';
                        if (result.success && result.edges.length) {
                            const title = document.createElement('strong');
                            title.textContent = `Dependencies of ${lib}:`;
                            depsContainer.appendChild(title);
                            const ul = document.createElement('ul');
                            ul.style.cssText = 'margin:0.2em 0 0 1em;padding:0;list-style:disc;';
                            const shown = new Set();
                            result.edges.forEach(e => {
                                const dep = e.to !== lib ? e.to : e.from;
                                if (dep === lib || shown.has(dep)) return;
                                shown.add(dep);
                                const li = document.createElement('li');
                            const badge = { PUBLIC: '🔗', PRIVATE: '🔒', INTERFACE: '📐', shared_lib: '📦', executable: '⚙️', interface_lib: '🔗', static_lib: '🧱', utility: '🔧', dependency: '•' };
                            li.textContent = (badge[e.kind] || '•') + ' ' + dep + (e.kind ? ` (${e.kind.toLowerCase()})` : '');
                                ul.appendChild(li);
                            });
                            if (!shown.size) {
                                const li = document.createElement('li');
                                li.textContent = '(no external dependencies)';
                                ul.appendChild(li);
                            }
                            depsContainer.appendChild(ul);
                            statusSpan.textContent = `${shown.size} dep(s)`;
                            statusSpan.style.color = '#555';
                        } else if (result.success) {
                            depsContainer.textContent = `${lib} has no external dependencies.`;
                            statusSpan.textContent = '';
                        } else {
                            depsContainer.style.color = 'red';
                            depsContainer.textContent = result.output || 'Failed to load dependencies.';
                            if (result.available_targets?.length) {
                                depsContainer.textContent += ` Available targets: ${result.available_targets.slice(0, 10).join(', ')}`;
                            }
                            statusSpan.textContent = 'Deps failed';
                            statusSpan.style.color = 'red';
                        }
                        row.after(depsContainer);
                    } catch (err) {
                        statusSpan.textContent = 'Error: ' + err;
                        statusSpan.style.color = 'red';
                    } finally {
                        depsBtn.disabled = false;
                    }
                });

                row.appendChild(label);
                row.appendChild(buildBtn);
                row.appendChild(depsBtn);
                row.appendChild(statusSpan);
                libsDiv.appendChild(row);
            });

            groupDiv.appendChild(libsDiv);
            this.appendChild(groupDiv);
        });
    }
}

customElements.define('groups-libraries', GroupsLibraries);
