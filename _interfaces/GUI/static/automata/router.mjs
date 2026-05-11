// router.mjs
// Minimal client-side router for automata components

const routes = {};

export function registerRoute(path, component) {
    routes[path] = component;
}

export function navigate(path) {
    window.history.pushState({}, '', path);
    renderRoute(path);
}

function renderRoute(path) {
    const component = routes[path] || routes['/404'];
    if (component) {
        document.getElementById('app').innerHTML = '';
        component();
    }
}

window.addEventListener('popstate', () => {
    renderRoute(window.location.pathname);
});

export function startRouter() {
    renderRoute(window.location.pathname);
}
