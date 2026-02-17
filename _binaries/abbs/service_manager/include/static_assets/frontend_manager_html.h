// Auto-generated from frontend_manager.html
#pragma once

namespace resources {
    constexpr const char* frontend_manager_html = R"HTML(
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Frontend Manager</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: #f0f0f0; }
        .header { background: #2c3e50; color: white; padding: 1rem 2rem; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header h1 { font-size: 1.5rem; }
        .back-btn { position: absolute; top: 1rem; right: 2rem; background: #3498db; color: white; padding: 0.5rem 1rem; text-decoration: none; border-radius: 4px; }
        .container { max-width: 1400px; margin: 2rem auto; padding: 0 2rem; }
        .section-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 2rem; }
        .section-title { font-size: 1.5rem; color: #2c3e50; }
        .add-btn { background: #27ae60; color: white; border: none; padding: 0.75rem 1.5rem; font-size: 1rem; border-radius: 4px; cursor: pointer; display: flex; align-items: center; gap: 0.5rem; }
        .add-btn:hover { background: #229954; }
        .frontends-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); gap: 2rem; }
        .frontend-card { background: white; padding: 1.5rem; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .frontend-header { display: flex; justify-content: space-between; align-items: start; margin-bottom: 1rem; }
        .frontend-name { font-size: 1.2rem; font-weight: bold; color: #2c3e50; }
        .frontend-status { padding: 0.25rem 0.75rem; border-radius: 12px; font-size: 0.8rem; }
        .status-running { background: #d4edda; color: #155724; }
        .status-stopped { background: #f8d7da; color: #721c24; }
        .frontend-desc { color: #7f8c8d; margin-bottom: 1rem; }
        .frontend-info { display: flex; gap: 1rem; margin-bottom: 1rem; font-size: 0.9rem; color: #95a5a6; }
        .frontend-actions { display: flex; gap: 0.5rem; }
        .btn { padding: 0.5rem 1rem; border: none; border-radius: 4px; cursor: pointer; font-size: 0.9rem; }
        .btn-primary { background: #3498db; color: white; }
        .btn-danger { background: #e74c3c; color: white; }
        .modal { display: none; position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.5); align-items: center; justify-content: center; }
        .modal.active { display: flex; }
        .modal-content { background: white; padding: 2rem; border-radius: 8px; width: 90%; max-width: 500px; }
        .modal-title { font-size: 1.5rem; margin-bottom: 1.5rem; color: #2c3e50; }
        .form-group { margin-bottom: 1.5rem; }
        .form-group label { display: block; margin-bottom: 0.5rem; font-weight: bold; color: #2c3e50; }
        .form-group input, .form-group textarea { width: 100%; padding: 0.75rem; border: 1px solid #ddd; border-radius: 4px; font-size: 1rem; }
        .form-actions { display: flex; gap: 1rem; justify-content: flex-end; }
        .btn-success { background: #27ae60; color: white; }
        .btn-secondary { background: #95a5a6; color: white; }
    </style>
</head>
<body>
    <div class="header">
        <h1>🎨 Frontend Manager</h1>
        <a href="/" class="back-btn">← Back to Apps</a>
    </div>
    
    <div class="container">
        <div class="section-header">
            <h2 class="section-title">Frontends</h2>
            <button class="add-btn" onclick="showCreateModal()">
                <span style="font-size: 1.5rem;">+</span>
                <span>New Frontend</span>
            </button>
        </div>
        
        <div id="frontends-list" class="frontends-grid">
            <div style="grid-column: 1/-1; text-align: center; padding: 3rem; color: #95a5a6;">
                Loading frontends...
            </div>
        </div>
    </div>
    
    <div id="create-modal" class="modal">
        <div class="modal-content">
            <h2 class="modal-title">Create New Frontend</h2>
            <form id="create-form" onsubmit="createFrontend(event)">
                <div class="form-group">
                    <label>Frontend Name *</label>
                    <input type="text" name="name" required placeholder="e.g., dashboard-frontend" pattern="[a-z0-9-]+">
                    <small style="color: #95a5a6;">Lowercase letters, numbers, and hyphens only</small>
                </div>
                <div class="form-group">
                    <label>Description</label>
                    <textarea name="description" rows="3" placeholder="Brief description of the frontend"></textarea>
                </div>
                <div class="form-group">
                    <label>Port *</label>
                    <input type="number" name="port" required value="3000" min="3000" max="9999">
                </div>
                <div class="form-group">
                    <label>Backend Port</label>
                    <input type="number" name="backend_port" value="9002" min="1024" max="65535">
                    <small style="color: #95a5a6;">Port for API proxy (leave blank if no backend)</small>
                </div>
                <div class="form-actions">
                    <button type="button" class="btn btn-secondary" onclick="hideCreateModal()">Cancel</button>
                    <button type="submit" class="btn btn-success">Create Frontend</button>
                </div>
            </form>
        </div>
    </div>
    
    <script>
        function loadFrontends() {
            fetch('/api/frontends/list')
                .then(r => r.json())
                .then(data => {
                    const container = document.getElementById('frontends-list');
                    if (data.frontends.length === 0) {
                        container.innerHTML = '<div style="grid-column: 1/-1; text-align: center; padding: 3rem; color: #95a5a6;">No frontends created yet. Click "+ New Frontend" to get started.</div>';
                        return;
                    }
                    
                    container.innerHTML = data.frontends.map(f => `
                        <div class="frontend-card">
                            <div class="frontend-header">
                                <div class="frontend-name">${f.name}</div>
                                <span class="frontend-status status-${f.status}">${f.status}</span>
                            </div>
                            <div class="frontend-desc">${f.description}</div>
                            <div class="frontend-info">
                                <span>📡 Port: ${f.port}</span>
                            </div>
                            <div class="frontend-actions">
                                <button class="btn btn-primary" onclick="window.open('http://localhost:${f.port}', '_blank')">Open</button>
                                <button class="btn btn-primary" onclick="window.open('/frontends/${f.name}', '_blank')">Files</button>
                            </div>
                        </div>
                    `).join('');
                })
                .catch(err => {
                    console.error('Failed to load frontends:', err);
                    document.getElementById('frontends-list').innerHTML = '<div style="grid-column: 1/-1; text-align: center; padding: 3rem; color: #e74c3c;">Failed to load frontends</div>';
                });
        }
        
        function showCreateModal() {
            document.getElementById('create-modal').classList.add('active');
        }
        
        function hideCreateModal() {
            document.getElementById('create-modal').classList.remove('active');
            document.getElementById('create-form').reset();
        }
        
        function createFrontend(event) {
            event.preventDefault();
            const formData = new FormData(event.target);
            const data = Object.fromEntries(formData.entries());
            
            fetch('/api/frontends/create', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: new URLSearchParams(data)
            })
            .then(r => r.json())
            .then(result => {
                if (result.success) {
                    alert('✅ Frontend created successfully!\n\nPath: ' + result.path + '\n\nTo start it:\ncd ' + result.path + '\nbun install\nbun dev');
                    hideCreateModal();
                    loadFrontends();
                } else {
                    alert('❌ Error: ' + (result.error || 'Unknown error'));
                }
            })
            .catch(err => {
                console.error('Failed to create frontend:', err);
                alert('❌ Failed to create frontend');
            });
        }
        
        // Load frontends on page load
        loadFrontends();
        
        // Close modal on backdrop click
        document.getElementById('create-modal').addEventListener('click', function(e) {
            if (e.target === this) hideCreateModal();
        });
    </script>
</body>
</html>

)HTML";
}
