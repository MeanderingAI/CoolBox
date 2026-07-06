// Auto-generated from service_manager.html.bak
#pragma once

namespace resources {
    constexpr const char* service_manager.html_bak = R"BAK(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Service Manager - MATLAB Platform</title>
        <style>
            /* SWIPEABLE TABLES */
            .swipe-table-container {
                overflow-x: auto;
                -webkit-overflow-scrolling: touch;
                margin-bottom: 32px;
                background: rgba(255,255,255,0.03);
                border-radius: 10px;
                box-shadow: 0 2px 8px rgba(139,92,246,0.08);
                padding: 8px 0;
                scrollbar-width: thin; /* Firefox */
            }
            .swipe-table-container::-webkit-scrollbar {
                height: 8px;
                background: rgba(139,92,246,0.05);
            }
            .swipe-table-container::-webkit-scrollbar-thumb {
                background: rgba(139,92,246,0.15);
                border-radius: 8px;
            }
            .swipe-table {
                min-width: 600px;
                width: 100%;
            }
            /* ...add more styles as needed... */
        </style>
    </head>
    <body>
        <div class="container">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:24px">
                <h1 style="font-size:2rem;font-weight:700;">Service Manager</h1>
                <button class="btn btn-primary" id="rebuildAllBtn" style="font-size:15px;padding:10px 18px;">Rebuild All</button>
            </div>
            <!-- Add your main content here -->
        </div>
        <script>
        document.getElementById('rebuildAllBtn').onclick = async function() {
            if (!confirm('Rebuild all shared libraries and binaries?')) return;
            this.disabled = true;
            this.textContent = 'Rebuilding...';
            try {
                const r = await fetch('/api/rebuild-all', {method:'POST'});
                if (!r.ok) throw new Error('Network error: ' + r.status);
                const d = await r.json();
                if (d.success) {
                    alert('All assets rebuilt!');
                    if (typeof fetchBinaries === 'function') fetchBinaries();
                    if (typeof fetchLibraries === 'function') fetchLibraries();
                } else {
                    alert('Build failed.');
                }
            } catch (e) {
                alert('Error: ' + (e.message || e));
            }
            this.disabled = false;
            this.textContent = 'Rebuild All';
        };
        </script>
    </body>
    </html>
    /* SWIPEABLE TABLES */
    .swipe-table-container {
        overflow-x: auto;
        -webkit-overflow-scrolling: touch;
        margin-bottom: 32px;
        background: rgba(255,255,255,0.03);
        border-radius: 10px;
        box-shadow: 0 2px 8px rgba(139,92,246,0.08);
        padding: 8px 0;
        scrollbar-width: thin; /* Firefox */
    }
    .swipe-table-container::-webkit-scrollbar {
        height: 8px;
        background: rgba(139,92,246,0.05);
    }
    .swipe-table-container::-webkit-scrollbar-thumb {
        background: rgba(139,92,246,0.15);
        border-radius: 8px;
    }
    .swipe-table {
        min-width: 600px;
        width: 100%;
    }
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Service Manager - MATLAB Platform</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #0a0a0a 0%, #1a1a2e 100%);
            color: #e0e0e0;
            padding: 20px;
            min-height: 100vh;
        }

        .container {
            max-width: 1400px;
            margin: 0 auto;
        }

        header {
            background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
            padding: 30px;
            border-radius: 12px;
            margin-bottom: 30px;
            box-shadow: 0 8px 32px rgba(99, 102, 241, 0.3);
            position: relative;
        }

        .hot-reload-banner {
            background: linear-gradient(90deg, #10b981 0%, #059669 100%);
            color: white;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            text-align: center;
            font-weight: 600;
            font-size: 16px;
            animation: pulse 2s ease-in-out infinite;
            box-shadow: 0 4px 20px rgba(16, 185, 129, 0.4);
        }

        @keyframes pulse {
            0%, 100% { transform: scale(1); }
            50% { transform: scale(1.02); }
        }

        h1 {
            font-size: 32px;
            font-weight: 700;
            margin-bottom: 10px;
        }

        .subtitle {
            font-size: 14px;
            opacity: 0.9;
        }

        .grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 20px;
            margin-bottom: 20px;
        }

        .card {
            background: rgba(255, 255, 255, 0.05);
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 12px;
            padding: 25px;
            box-shadow: 0 4px 16px rgba(0, 0, 0, 0.3);
        }

        .card h2 {
            font-size: 20px;
            margin-bottom: 20px;
            color: #a78bfa;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .full-width {
            grid-column: 1 / -1;
        }

        table {
            width: 100%;
            border-collapse: collapse;
        }

        thead {
            background: rgba(139, 92, 246, 0.1);
        }

        th {
            padding: 12px;
            text-align: left;
            font-weight: 600;
            font-size: 13px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            color: #a78bfa;
            border-bottom: 2px solid rgba(139, 92, 246, 0.3);
        }

        td {
            padding: 12px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.05);
        }

        tr:hover {
            background: rgba(255, 255, 255, 0.02);
        }

        .status {
            display: inline-flex;
            align-items: center;
            gap: 6px;
            padding: 4px 12px;
            border-radius: 20px;
            font-size: 12px;
            font-weight: 600;
        }

        .status-running {
            background: rgba(34, 197, 94, 0.2);
            color: #4ade80;
        }

        .status-stopped {
            background: rgba(239, 68, 68, 0.2);
            color: #f87171;
        }

        .status-dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            animation: pulse 2s infinite;
        }

        .status-running .status-dot {
            background: #4ade80;
        }

        .status-stopped .status-dot {
            background: #f87171;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        .btn {
            padding: 6px 14px;
            border-radius: 6px;
            border: none;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.2s;
            margin-right: 6px;
        }

        .btn-primary {
            background: linear-gradient(135deg, #6366f1 0%, #8b5cf6 100%);
            color: white;
        }

        .btn-primary:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(99, 102, 241, 0.4);
        }

        .btn-danger {
            background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
            color: white;
        }

        .btn-danger:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(239, 68, 68, 0.4);
        }

        .btn-secondary {
            background: rgba(255, 255, 255, 0.1);
            color: #e0e0e0;
        }

        .btn-secondary:hover {
            background: rgba(255, 255, 255, 0.15);
        }

        .btn-success {
            background: linear-gradient(135deg, #10b981 0%, #059669 100%);
            color: white;
        }

        .btn-success:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(16, 185, 129, 0.4);
        }

        .btn-warning {
            background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%);
            color: white;
        }

        .btn-warning:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(245, 158, 11, 0.4);
        }

        .btn-info {
            background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
            color: white;
        }

        .btn-info:hover {
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(59, 130, 246, 0.4);
        }

        .method-badge {
            display: inline-block;
            padding: 4px 10px;
            border-radius: 4px;
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 0.5px;
        }

        .method-get {
            background: rgba(34, 197, 94, 0.2);
            color: #4ade80;
        }

        .method-post {
            background: rgba(251, 191, 36, 0.2);
            color: #fbbf24;
        }

        .method-put {
            background: rgba(59, 130, 246, 0.2);
            color: #60a5fa;
        }

        .method-delete {
            background: rgba(239, 68, 68, 0.2);
            color: #f87171;
        }

        .path-cell {
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 13px;
            color: #a78bfa;
        }

        .timestamp {
            font-family: 'SF Mono', Monaco, 'Courier New', monospace;
            font-size: 12px;
            color: #9ca3af;
        }

        .stats {
            display: flex;
            gap: 20px;
            margin-bottom: 20px;
        }

        .stat-card {
            flex: 1;
            background: rgba(139, 92, 246, 0.1);
            padding: 20px;
            border-radius: 8px;
            border: 1px solid rgba(139, 92, 246, 0.2);
        }

        .stat-label {
            font-size: 12px;
            color: #9ca3af;
            margin-bottom: 8px;
        }

        .stat-value {
            font-size: 28px;
            font-weight: 700;
            color: #a78bfa;
        }

        .refresh-indicator {
            display: inline-block;
            margin-left: 10px;
            color: #4ade80;
            font-size: 12px;
            animation: fadeIn 0.3s;
        }

        @keyframes fadeIn {
            from { opacity: 0; }
            to { opacity: 1; }
        }

        .console-section {
            background: #0d1117;
            border-radius: 8px;
            padding: 15px;
            max-height: 400px;
            overflow-y: auto;
            font-family: 'Monaco', 'Menlo', 'Courier New', monospace;
            font-size: 13px;
            line-height: 1.5;
        }

        .console-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            margin-bottom: 10px;
            padding-bottom: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.1);
        }

        .console-title {
            color: #a78bfa;
            font-size: 14px;
            font-weight: 600;
        }

        .console-line {
            padding: 2px 0;
            color: #e0e0e0;
            white-space: pre-wrap;
            word-wrap: break-word;
        }

        .console-empty {
            color: #666;
            font-style: italic;
            text-align: center;
            padding: 40px 20px;
        }

        .selected-row {
            background: rgba(139, 92, 246, 0.15) !important;
            border-left: 3px solid #8b5cf6;
        }

        .empty-state {
            text-align: center;
            padding: 40px;
            color: #6b7280;
        }

        .empty-state-icon {
            font-size: 48px;
            margin-bottom: 16px;
        }
    </style>
</head>
<body>
                <div class="container">
                    <div class="card full-width">
                        <h2>📚 Shared Libraries (.dylib)</h2>
                        <div class="swipe-table-container">
                            <table class="swipe-table" id="dylibs-table">
                                <thead><tr><th>Name</th><th>Path</th><th>Size</th><th>Last Modified</th></tr></thead>
                                <tbody id="dylibs-list"><tr><td colspan="4">Loading...</td></tr></tbody>
                            </table>
                        </div>
                    </div>
                    <div class="card full-width">
                        <h2>🧪 Demos</h2>
                        <div class="swipe-table-container">
                            <table class="swipe-table" id="demos-table">
                                <thead><tr><th>Name</th><th>Path</th><th>Size</th><th>Last Modified</th></tr></thead>
                                <tbody id="demos-list"><tr><td colspan="4">Loading...</td></tr></tbody>
                            </table>
                        </div>
                    </div>
                    <div class="card full-width">
                        <h2>🛠️ Services</h2>
                        <div class="swipe-table-container">
                            <table class="swipe-table" id="services-table">
                                <thead><tr><th>Name</th><th>Path</th><th>Size</th><th>Last Modified</th></tr></thead>
                                <tbody id="services-list"><tr><td colspan="4">Loading...</td></tr></tbody>
                            </table>
                        </div>
                    </div>
                    <div class="card full-width">
                        <h2>🖥️ Apps</h2>
                        <div class="swipe-table-container">
                            <table class="swipe-table" id="apps-table">
                                <thead><tr><th>Name</th><th>Path</th><th>Size</th><th>Last Modified</th></tr></thead>
                                <tbody id="apps-list"><tr><td colspan="4">Loading...</td></tr></tbody>
                            </table>
                        </div>
                    </div>
                </div>
        <div class="container">
            <div style="display:flex;align-items:center;justify-content:space-between;margin-bottom:24px">
                <h1 style="font-size:2rem;font-weight:700;">Service Manager</h1>
                <button class="btn btn-primary" id="rebuildAllBtn" style="font-size:15px;padding:10px 18px;">Rebuild All</button>
            </div>
                document.getElementById('rebuildAllBtn').onclick = async function() {
                    if (!confirm('Rebuild all shared libraries and binaries?')) return;
                    this.disabled = true;
                    this.textContent = 'Rebuilding...';
                    try {
                        const r = await fetch('/api/rebuild-all', {method:'POST'});
                        if (!r.ok) throw new Error('Network error: ' + r.status);
                        const d = await r.json();
                        if (d.success) {
                            alert('All assets rebuilt!');
                            if (typeof fetchBinaries === 'function') fetchBinaries();
                            if (typeof fetchLibraries === 'function') fetchLibraries();
                        } else {
                            alert('Build failed.');
                        }
                    } catch (e) {
                        alert('Error: ' + (e.message || e));
                    }
                    this.disabled = false;
                    this.textContent = 'Rebuild All';
                };
        <!-- API Documentation Menu -->
        <nav id="api-docs-menu" style="margin-bottom: 24px; background: rgba(99,102,241,0.08); border-radius: 8px; padding: 18px 24px; box-shadow: 0 2px 8px rgba(99,102,241,0.08);">
            <h2 style="font-size: 18px; color: #a78bfa; margin-bottom: 10px;">📚 API Documentation</h2>
            <ul id="api-docs-list" style="list-style: none; padding-left: 0; display: flex; flex-wrap: wrap; gap: 18px;">
                <li>Loading documentation...</li>
            </ul>
        </nav>
        <!-- API Routes Menu -->
        <nav id="api-routes-menu" style="margin-bottom: 24px; background: rgba(34,197,94,0.08); border-radius: 8px; padding: 18px 24px; box-shadow: 0 2px 8px rgba(34,197,94,0.08);">
            <h2 style="font-size: 18px; color: #4ade80; margin-bottom: 10px;">🛣️ Exposed API Routes</h2>
            <table id="api-routes-table" style="width:100%;border-collapse:collapse;font-size:15px;">
                <thead><tr style="background:rgba(34,197,94,0.12)"><th style="padding:8px 12px;text-align:left;color:#22d3ee">Method</th><th style="padding:8px 12px;text-align:left;color:#22d3ee">Path</th><th style="padding:8px 12px;text-align:left;color:#22d3ee">Description</th></tr></thead>
                <tbody id="api-routes-list"><tr><td colspan="3" style="padding:12px;color:#9ca3af">Loading routes...</td></tr></tbody>
            </table>
        </nav>
        <script>
                // Fetch and render dylibs, demos, services, and apps
                async function fetchAndRenderTable(endpoint, tbodyId) {
                    const tbody = document.getElementById(tbodyId);
                    tbody.innerHTML = '<tr><td colspan="4">Loading...</td></tr>';
                    try {
                        const res = await fetch(endpoint);
                        const data = await res.json();
                        if (!Array.isArray(data) || data.length === 0) {
                            tbody.innerHTML = '<tr><td colspan="4">No data found</td></tr>';
                            return;
                        }
                        tbody.innerHTML = data.map(item =>
                            `<tr><td>${item.name}</td><td style="font-size:12px;">${item.path}</td><td>${item.size}</td><td>${item.last_modified}</td></tr>`
                        ).join('');
                    } catch (e) {
                        tbody.innerHTML = `<tr><td colspan="4">Error: ${e.message}</td></tr>`;
                    }
                }

                function loadAllSwipeTables() {
                    fetchAndRenderTable('/api/dylibs', 'dylibs-list');
                    fetchAndRenderTable('/api/demos', 'demos-list');
                    fetchAndRenderTable('/api/services-list', 'services-list');
                    fetchAndRenderTable('/api/apps', 'apps-list');
                }
                window.addEventListener('DOMContentLoaded', loadAllSwipeTables);
        async function loadApiDocsMenu() {
            const list = document.getElementById('api-docs-list');

                <table id="frontend-table">
                    <thead>
                        <tr>
                            <th>ID</th>
                            <th>Service</th>
                            <th>Port</th>
                            <th>Status</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="frontend-services">
                        <tr>
                            <td colspan="5" class="empty-state">
                                <div class="empty-state-icon">⏳</div>
                                <div>Loading services...</div>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <div class="card full-width">
                <h2>🖥️ Service Console</h2>
                <div class="console-section">
                    <div class="console-header">
                        <div class="console-title" id="console-service-name">No service selected</div>
                        <div>
                            <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px; margin-right: 5px;" onclick="exportConsoleLog()">📊 Export CSV</button>
                            <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px; margin-right: 5px;" onclick="openConsoleWindow()">🪟 Pop Out</button>
                            <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px;" onclick="clearConsole()">Clear</button>
                        </div>
                    </div>
                    <div id="console-output" class="console-empty">
                        Click on a service row to view its output
                    </div>
                </div>
            </div>

            <div class="card full-width">
                <h2 style="display: flex; align-items: center; justify-content: space-between;">
                    <span>
                        📊 Recent HTTP Requests
                        <span class="refresh-indicator" id="refresh-indicator" style="display: none;">● Live</span>
                    </span>
                    <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px;" onclick="exportRequestLog()">📊 Export CSV</button>
                </h2>
                <table>
                    <thead>
                        <tr>
                            <th>Timestamp</th>
                            <th>Method</th>
                            <th>Path</th>
                            <th>Thread ID</th>
                        </tr>
                    </thead>
                    <tbody id="request-log">
                        <tr>
                            <td colspan="4" class="empty-state">
                                <div class="empty-state-icon">📭</div>
                                <div>No requests yet. Make a request to see it appear here!</div>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <div class="card full-width">
                <h2 style="display: flex; align-items: center; justify-content: space-between;">
                    <span>
                        � Hot-Recompile Watchdog
                        <span id="watchdog-status-indicator" style="font-size: 11px; margin-left: 10px;"></span>
                    </span>
                    <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px;" onclick="refreshWatchdogStatus()">🔄 Refresh</button>
                </h2>
                
                <div style="margin-bottom: 15px; padding: 12px; background: rgba(34, 197, 94, 0.1); border-radius: 6px; font-size: 13px; line-height: 1.6;">
                    <strong>About Watchdog:</strong> Automatically monitors C++ service source files and headers in the services/ directory.
                    When changes are detected, it rebuilds the affected services and restarts them if they were running.
                    <div style="margin-top: 8px; font-size: 12px; color: #9ca3af;">
                        Check interval: <code style="background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 3px;">2 seconds</code> | 
                        Auto-rebuild: <code style="background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 3px;">Enabled</code>
                    </div>
                </div>

                <div class="stats" id="watchdog-stats" style="margin-bottom: 15px;">
                    <div class="stat-card">
                        <div class="stat-label">Uptime</div>
                        <div class="stat-value" id="watchdog-uptime">--</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Total Checks</div>
                        <div class="stat-value" id="watchdog-checks">0</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Rebuilds</div>
                        <div class="stat-value" id="watchdog-rebuilds">0</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-label">Restarts</div>
                        <div class="stat-value" id="watchdog-restarts">0</div>
                    </div>
                </div>

                <table id="watchdog-table">
                    <thead>
                        <tr>
                            <th>File Path</th>
                            <th>Type</th>
                            <th>Last Modified</th>
                            <th>Affects Services</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="watchdog-files">
                        <tr>
                            <td colspan="5" class="empty-state">
                                <div class="empty-state-icon">⏳</div>
                                <div>Loading watchdog status...</div>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>

            <div class="card full-width">
                <h2 style="display: flex; align-items: center; justify-content: space-between;">
                    <span>
                        �🔀 nginx Routing Configuration
                        <span id="nginx-status-indicator" style="font-size: 11px; margin-left: 10px;"></span>
                    </span>
                    <div>
                        <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px; margin-right: 5px;" onclick="refreshNginxStatus()">🔄 Refresh</button>
                        <button class="btn btn-primary" style="font-size: 11px; padding: 5px 10px;" onclick="reloadNginx()">⚡ Reload nginx</button>
                    </div>
                </h2>
                
                <div style="margin-bottom: 15px; padding: 12px; background: rgba(99, 102, 241, 0.1); border-radius: 6px; font-size: 13px; line-height: 1.6;">
                    <strong>About nginx Routing:</strong> When enabled, nginx acts as a reverse proxy routing external ports (8001-8006) to internal service ports (9001-9006).
                    This provides load balancing, SSL termination, and professional production-ready service routing.
                    <div style="margin-top: 8px; font-size: 12px; color: #9ca3af;">
                        Config directory: <code style="background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 3px;">/usr/local/etc/nginx/servers/</code>
                    </div>
                </div>

                <table id="nginx-table">
                    <thead>
                        <tr>
                            <th>Service ID</th>
                            <th>Service Name</th>
                            <th>Internal Port</th>
                            <th>nginx Port</th>
                            <th>Config Status</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="nginx-services">
                        <tr>
                            <td colspan="6" class="empty-state">
                                <div class="empty-state-icon">⏳</div>
                                <div>Loading nginx configuration...</div>
                            </td>
                        </tr>
                    </tbody>
                </table>

                <div id="nginx-editor" style="display: none; margin-top: 20px; padding: 15px; background: rgba(0, 0, 0, 0.3); border-radius: 8px;">
                    <h3 style="color: #a78bfa; font-size: 16px; margin-bottom: 10px;">
                        Edit Configuration for <span id="editor-service-name"></span>
                    </h3>
                    <textarea id="nginx-config-content" 
                              style="width: 100%; min-height: 400px; background: #0d1117; color: #e0e0e0; border: 1px solid rgba(255,255,255,0.1); border-radius: 6px; padding: 12px; font-family: 'Monaco', 'Menlo', 'Courier New', monospace; font-size: 13px; line-height: 1.5; resize: vertical;">
                    </textarea>
                    <div style="margin-top: 10px; display: flex; gap: 10px;">
                        <button class="btn btn-primary" onclick="saveNginxConfig()">💾 Save & Reload</button>
                        <button class="btn btn-secondary" onclick="closeNginxEditor()">❌ Cancel</button>
                        <div id="editor-status" style="margin-left: auto; padding: 8px 12px; border-radius: 6px; font-size: 12px; display: none;"></div>
                    </div>
                </div>
            </div>

            <div class="card full-width">
                <h2 style="display: flex; align-items: center; justify-content: space-between;">
                    <span>
                        📚 Shared Libraries (.so/.dylib)
                        <span id="libraries-count" style="font-size: 11px; margin-left: 10px; color: #9ca3af;">0 libraries</span>
                    </span>
                    <button class="btn btn-secondary" style="font-size: 11px; padding: 5px 10px;" onclick="fetchLibraries()">🔄 Refresh</button>
                </h2>
                
                <div style="margin-bottom: 15px; padding: 12px; background: rgba(34, 197, 94, 0.1); border-radius: 6px; font-size: 13px; line-height: 1.6;">
                    <strong>About Shared Libraries:</strong> These are the compiled C++ libraries (.so on Linux, .dylib on macOS) that provide core functionality.
                    You can rebuild individual libraries without recompiling the entire project.
                    <div style="margin-top: 8px; font-size: 12px; color: #9ca3af;">
                        Build directory: <code style="background: rgba(0,0,0,0.3); padding: 2px 6px; border-radius: 3px;">build/libraries/src/</code>
                    </div>
                </div>

                <table id="libraries-table">
                    <thead>
                        <tr>
                            <th>Library Name</th>
                            <th>Make Target</th>
                            <th>Size</th>
                            <th>Last Modified</th>
                            <th>Actions</th>
                        </tr>
                    </thead>
                    <tbody id="libraries-list">
                        <tr>
                            <td colspan="5" class="empty-state">
                                <div class="empty-state-icon">⏳</div>
                                <div>Loading libraries...</div>
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    </div>

    <script>
        let requestCount = 0;
        let services = {
            backend: [],
            frontend: []
        };
        let selectedServiceId = null;
        let requestLog = []; // Store all requests for export
        let consoleLog = []; // Store console output for export
        let nginxStatus = null; // Store nginx status
        let currentEditingServiceId = null; // Track which service config is being edited

        // Check if a service is actually responding
        async function checkServiceHealth(port) {
            try {
                const controller = new AbortController();
                const timeoutId = setTimeout(() => controller.abort(), 1000); // 1 second timeout
                
                const response = await fetch(`http://localhost:${port}/health`, {
                    signal: controller.signal,
                    mode: 'cors'
                });
                clearTimeout(timeoutId);
                
                if (response.ok) {
                    const data = await response.json();
                    return data.status === 'ok' || data.status === 'healthy';
                }
                return false;
            } catch (error) {
                // Service not responding
                return false;
            }
        }

        // Fetch actual service data from backend
        async function fetchServices() {
            try {
                const response = await fetch('/api/services');
                const data = await response.json();
                
                // Split services into backend and frontend
                services.backend = [];
                services.frontend = [];
                
                for (let id in data) {
                    const svc = data[id];
                    
                    // Check actual service health for backend services (IDs 2 and 3)
                    let actuallyRunning = svc.running;
                    if (svc.running && (id == 2 || id == 3)) {
                        actuallyRunning = await checkServiceHealth(svc.port);
                    }
                    
                    const serviceObj = {
                        id: parseInt(id),
                        name: svc.name,
                        port: svc.port,
                        status: actuallyRunning ? 'running' : 'stopped',
                        description: svc.description || '',
                        has_docs: (id == 2)  // Only Metrics Backend has API docs
                    };
                    
                    if (id <= 3) {
                        services.backend.push(serviceObj);
                    } else {
                        services.frontend.push(serviceObj);
                    }
                }
                
                renderServices();
            } catch (error) {
                console.error('Failed to fetch services:', error);
            }
        }

        function selectService(id) {
            selectedServiceId = id;
            const service = [...services.backend, ...services.frontend].find(s => s.id === id);
            if (service) {
                document.getElementById('console-service-name').textContent = service.name + ' (ID: ' + id + ')';
                fetchServiceOutput(id);
            }
            renderServices();
        }

        async function fetchServiceOutput(id) {
            try {
                const response = await fetch(`/api/service/output?id=${id}`);
                const data = await response.json();
                
                const consoleOutput = document.getElementById('console-output');
                if (data.lines && data.lines.length > 0) {
                    consoleOutput.innerHTML = data.lines.map(line => 
                        `<div class="console-line">${escapeHtml(line)}</div>`
                    ).join('');
                    // Auto-scroll to bottom
                    consoleOutput.scrollTop = consoleOutput.scrollHeight;
                } else {
                    consoleOutput.innerHTML = '<div class="console-empty">No output available</div>';
                }
            } catch (error) {
                console.error('Failed to fetch service output:', error);
                document.getElementById('console-output').innerHTML = 
                    '<div class="console-empty">Error fetching output</div>';
            }
        }

        function clearConsole() {
            document.getElementById('console-output').innerHTML = 
                '<div class="console-empty">Console cleared</div>';
        }

        function openConsoleWindow() {
            if (selectedServiceId !== null) {
                const url = `/api/service/console?id=${selectedServiceId}`;
                window.open(url, `console_${selectedServiceId}`, 'width=800,height=600,menubar=no,toolbar=no,location=no,status=no');
            } else {
                alert('Please select a service first');
            }
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        // Export console log to CSV
        function exportConsoleLog() {
            if (selectedServiceId === null) {
                alert('Please select a service first');
                return;
            }
            
            const service = [...services.backend, ...services.frontend].find(s => s.id === selectedServiceId);
            if (!service) return;
            
            // Get current console output
            const consoleOutput = document.getElementById('console-output');
            const lines = Array.from(consoleOutput.querySelectorAll('.console-line'));
            
            if (lines.length === 0) {
                alert('No console output to export');
                return;
            }
            
            // Create CSV content
            let csv = 'Timestamp,Service ID,Service Name,Line Number,Output\n';
            const timestamp = new Date().toISOString();
            
            lines.forEach((line, index) => {
                const text = line.textContent.replace(/"/g, '""'); // Escape quotes
                csv += `"${timestamp}",${selectedServiceId},"${service.name}",${index + 1},"${text}"\n`;
            });
            
            // Download CSV
            const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
            const link = document.createElement('a');
            const url = URL.createObjectURL(blob);
            link.setAttribute('href', url);
            link.setAttribute('download', `console_${service.name}_${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.csv`);
            link.style.visibility = 'hidden';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }

        // Export request log to CSV
        function exportRequestLog() {
            if (requestLog.length === 0) {
                alert('No requests to export');
                return;
            }
            
            // Create CSV content
            let csv = 'Timestamp,Method,Path,Thread ID\n';
            
            requestLog.forEach(req => {
                csv += `"${req.timestamp}","${req.method}","${req.path}","${req.threadId}"\n`;
            });
            
            // Download CSV
            const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
            const link = document.createElement('a');
            const url = URL.createObjectURL(blob);
            link.setAttribute('href', url);
            link.setAttribute('download', `http_requests_${new Date().toISOString().slice(0,19).replace(/:/g,'-')}.csv`);
            link.style.visibility = 'hidden';
            document.body.appendChild(link);
            link.click();
            document.body.removeChild(link);
        }

        function renderServices() {
            const backendBody = document.getElementById('backend-services');
            const frontendBody = document.getElementById('frontend-services');

            backendBody.innerHTML = services.backend.map(svc => `
                <tr class="${selectedServiceId === svc.id ? 'selected-row' : ''}" onclick="selectService(${svc.id})" style="cursor: pointer;">
                    <td>${svc.id}</td>
                    <td><strong>${svc.name}</strong></td>
                    <td><input type="number" id="port-${svc.id}" value="${svc.port}" style="width: 80px; padding: 4px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); color: white; border-radius: 4px;" onclick="event.stopPropagation()"></td>
                    <td>
                        <span class="status ${svc.status === 'running' ? 'status-running' : 'status-stopped'}">
                            <span class="status-dot"></span>
                            ${svc.status === 'running' ? 'RUNNING' : 'STOPPED'}
                        </span>
                    </td>
                    <td>
                        ${svc.status === 'stopped' ? 
                            `<button class="btn btn-primary" onclick="startService(${svc.id})">Start</button>
                             <button class="btn btn-secondary" style="font-size: 11px;" onclick="autoAssignPort(${svc.id})">🔄 Auto-Port</button>` :
                            `<button class="btn btn-danger" onclick="stopService(${svc.id})">Stop</button>                             <button class="btn btn-danger" onclick="killService(${svc.id})" style="background: #8B0000;">Kill</button>                             <button class="btn btn-secondary" onclick="restartService(${svc.id})">Restart</button>
                             <button class="btn btn-warning" onclick="rebuildService(${svc.id})">🔨 Rebuild</button>
                             ${svc.has_docs ? `<button class="btn btn-info" onclick="openDocs(${svc.port})">📚 Docs</button>` : ''}
                             <button class="btn btn-success" onclick="openService(${svc.port})">Open</button>`
                        }
                    </td>
                </tr>
            `).join('');

            frontendBody.innerHTML = services.frontend.map(svc => `
                <tr class="${selectedServiceId === svc.id ? 'selected-row' : ''}" onclick="selectService(${svc.id})" style="cursor: pointer;">
                    <td>${svc.id}</td>
                    <td><strong>${svc.name}</strong></td>
                    <td><input type="number" id="port-${svc.id}" value="${svc.port}" style="width: 80px; padding: 4px; background: rgba(255,255,255,0.1); border: 1px solid rgba(255,255,255,0.2); color: white; border-radius: 4px;" onclick="event.stopPropagation()"></td>
                    <td>
                        <span class="status ${svc.status === 'running' ? 'status-running' : 'status-stopped'}">
                            <span class="status-dot"></span>
                            ${svc.status === 'running' ? 'RUNNING' : 'STOPPED'}
                        </span>
                    </td>
                    <td>
                        ${svc.status === 'stopped' ? 
                            `<button class="btn btn-primary" onclick="startService(${svc.id})">Start</button>` :
                            `<button class="btn btn-danger" onclick="stopService(${svc.id})">Stop</button>
                             <button class="btn btn-secondary" onclick="restartService(${svc.id})">Restart</button>
                             <button class="btn btn-warning" onclick="rebuildService(${svc.id})">🔨 Rebuild</button>
                             <button class="btn btn-success" onclick="openService(${svc.port})">Open</button>`
                        }
                    </td>
                </tr>
            `).join('');

            updateStats();
        }

        function updateStats() {
            const backendRunning = services.backend.filter(s => s.status === 'running').length;
            const frontendRunning = services.frontend.filter(s => s.status === 'running').length;
            
            document.getElementById('backend-count').textContent = `${backendRunning}/3`;
            document.getElementById('frontend-count').textContent = `${frontendRunning}/3`;
            document.getElementById('request-count').textContent = requestCount;
        }

        async function startService(id) {
            selectService(id);
            
            // Get port from input field
            const portInput = document.getElementById(`port-${id}`);
            const port = portInput ? parseInt(portInput.value) : null;
            
            try {
                const body = port ? { service_id: id, port: port } : { service_id: id };
                const response = await fetch('/api/service/start', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(body)
                });
                const data = await response.json();
                if (data.success) {
                    // Service list will be updated on next fetch
                    setTimeout(() => fetchServices(), 500);
                }
            } catch (error) {
                console.error('Failed to start service:', error);
            }
        }

        async function stopService(id) {
            try {
                const response = await fetch('/api/service/stop', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ service_id: id })
                });
                const data = await response.json();
                if (data.success) {
                    setTimeout(() => fetchServices(), 500);
                }
            } catch (error) {
                console.error('Failed to stop service:', error);
            }
        }

        async function killService(id) {
            const service = services.backend.find(s => s.id === id) || services.frontend.find(s => s.id === id);
            if (!service) return;
            
            if (confirm(`Force kill ${service.name}?\\n\\nThis will immediately terminate the process using SIGKILL.`)) {
                try {
                    const response = await fetch('/api/service/kill', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ service_id: id })
                    });
                    const data = await response.json();
                    if (data.success) {
                        setTimeout(() => fetchServices(), 500);
                    }
                } catch (error) {
                    console.error('Failed to kill service:', error);
                }
            }
        }

        async function autoAssignPort(id) {
            const service = services.backend.find(s => s.id === id) || services.frontend.find(s => s.id === id);
            if (!service) return;
            
            if (confirm(`Auto-assign a free port for ${service.name}?\\n\\nThe current port (${service.port}) appears to be in use. A free port will be automatically selected.`)) {
                try {
                    const response = await fetch('/api/service/autoport', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({ service_id: id })
                    });
                    const data = await response.json();
                    if (data.success) {
                        alert('✓ Free port assigned successfully! Check the console output for the new port number.');
                        setTimeout(() => {
                            fetchServices();
                            fetchServiceOutput(id);
                        }, 500);
                    } else {
                        alert('✗ Failed to find a free port. Try manually specifying a port number.');
                    }
                } catch (error) {
                    console.error('Failed to auto-assign port:', error);
                    alert('✗ Error: ' + error.message);
                }
            }
        }

        function restartService(id) {
            stopService(id);
            setTimeout(() => startService(id), 500);
        }

        function openService(port) {
            const url = `http://localhost:${port}`;
            window.open(url, '_blank');
        }

        function openDocs(port) {
            const url = `http://localhost:${port}/api/docs`;
            window.open(url, '_blank');
        }

        function rebuildService(id) {
            const service = services.backend.find(s => s.id === id) || services.frontend.find(s => s.id === id);
            if (!service) return;
            
            if (confirm(`Rebuild ${service.name}?\\n\\nThis will recompile the service and may take a moment.`)) {
                console.log('Rebuilding service', id);
                
                fetch('/api/rebuild', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ service_id: id })
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        alert('Service rebuilt successfully!\\n\\n' + data.message);
                    } else {
                        alert('Rebuild failed: ' + data.message);
                    }
                })
                .catch(error => {
                    console.error('Rebuild error:', error);
                    alert('Failed to rebuild service: ' + error.message);
                });
            }
        }

        function addRequest(method, path, threadId) {
            const tbody = document.getElementById('request-log');
            const now = new Date();
            const timestamp = now.toTimeString().split(' ')[0] + '.' + now.getMilliseconds().toString().padStart(3, '0');
            
            // Store in log array for export
            requestLog.push({
                timestamp: now.toISOString(),
                method: method,
                path: path,
                threadId: threadId
            });
            
            // Keep only last 1000 requests in memory
            if (requestLog.length > 1000) {
                requestLog.shift();
            }
            
            const methodClass = 'method-' + method.toLowerCase();
            
            const row = document.createElement('tr');
            row.innerHTML = `
                <td class="timestamp">${timestamp}</td>
                <td><span class="method-badge ${methodClass}">${method}</span></td>
                <td class="path-cell">${path}</td>
                <td class="timestamp">${threadId}</td>
            `;
            
            // Insert at top
            if (tbody.firstChild && tbody.firstChild.querySelector('.empty-state')) {
                tbody.innerHTML = '';
            }
            tbody.insertBefore(row, tbody.firstChild);
            
            // Keep only last 50 requests
            while (tbody.children.length > 50) {
                tbody.removeChild(tbody.lastChild);
            }
            
            requestCount++;
            updateStats();
            
            // Show refresh indicator
            const indicator = document.getElementById('refresh-indicator');
            indicator.style.display = 'inline-block';
            setTimeout(() => {
                indicator.style.display = 'none';
            }, 1000);
        }

        // Simulate some requests for demo
        function simulateRequest() {
            const methods = ['GET', 'POST', 'PUT', 'DELETE'];
            const paths = ['/', '/api/data', '/api/users', '/app/cache', '/app/frontends', '/api/services'];
            const method = methods[Math.floor(Math.random() * methods.length)];
            const path = paths[Math.floor(Math.random() * paths.length)];
            const threadId = '0x' + Math.floor(Math.random() * 0xFFFFFF).toString(16).padStart(6, '0');
            
            addRequest(method, path, threadId);
        }

        // Initialize
        fetchServices();

        // Auto-refresh every 2 seconds
        setInterval(fetchServices, 2000);

        // Auto-refresh console output if a service is selected
        setInterval(() => {
            if (selectedServiceId !== null) {
                fetchServiceOutput(selectedServiceId);
            }
        }, 1000);

        // Fetch nginx status on page load and every 5 seconds
        fetchNginxStatus();
        setInterval(fetchNginxStatus, 5000);

        // Fetch watchdog status on page load and every 5 seconds
        fetchWatchdogStatus();
        setInterval(fetchWatchdogStatus, 5000);

        // Fetch libraries on page load and every 10 seconds
        fetchLibraries();
        setInterval(fetchLibraries, 10000);

        // API Routes Functions
        async function fetchApiRoutes() {
            try {
                const r = await fetch('/api/routes');
                const d = await r.json();
                const tbody = document.getElementById('api-routes-list');
                if (!d.routes || !d.routes.length) {
                    tbody.innerHTML = '<tr><td colspan="3" style="padding:12px;color:#9ca3af">No routes found</td></tr>';
                    return;
                }
                tbody.innerHTML = '';
                d.routes.forEach(route => {
                    const tr = document.createElement('tr');
                    tr.innerHTML = `<td style="padding:8px 12px;color:#22d3ee;font-family:monospace;">${route.method}</td><td style="padding:8px 12px;font-family:monospace;">${route.path}</td><td style="padding:8px 12px;">${route.description}</td>`;
                    tbody.appendChild(tr);
                });
            } catch (e) {
                console.error('Failed to fetch API routes:', e);
            }
        }
        document.addEventListener('DOMContentLoaded', function() {
            fetchApiRoutes();
        });
    </script>
</body>
</html>

)BAK";
}
