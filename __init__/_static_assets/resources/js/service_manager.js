function formatBytes(bytes) {
    if (!bytes) return '0 B';
    const k = 1024, sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function formatDate(ts) {
    if (!ts) return '';
    const d = new Date(ts * 1000);
    return d.toLocaleString();
}

function showNotification(msg, type = 'info') {
  const nc = document.getElementById('notification-center');
  if (nc && typeof nc.addNotification === 'function') {
    nc.addNotification(msg, type);
  }
}

async function loadMakeHelpTables() {
  document.getElementById('loading-indicator').style.display = 'inline';
  try {
    const r = await fetch('/api/make_help');
    if (!r.ok) throw new Error('Network error: ' + r.status);
    const d = await r.json();
    document.getElementById('tables-root').tablesData = d;
  } catch (e) {
    // Optionally show error in the component
    showNotification('Failed to load data: ' + (e.message || e), 'error');
  }
  document.getElementById('loading-indicator').style.display = 'none';
}

document.getElementById('loading-indicator').style.display = 'inline';
document.getElementById('refreshBtn').onclick = loadMakeHelpTables;
window.addEventListener('DOMContentLoaded', loadMakeHelpTables);
