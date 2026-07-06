// applicationState.mjs
// Provides ApplicationState class for structured app state and cacheFetch utility

const _cache = new Map();

export class ApplicationState {
    // Static API endpoint links
    static DASHBOARD_URL = '/dashboard';
    static ITEMS_URL = '/items/';
    static GROUPS_URL = '/dashboard';

    static async cacheFetch(url, options) {
        if (_cache.has(url)) {
            return _cache.get(url);
        }
        const resp = await fetch(url, options);
        if (!resp.ok) throw new Error(`Failed to fetch ${url}: ${resp.status}`);
        const data = await resp.json();
        _cache.set(url, data);
        return data;
    }

    static async getAllGroupsAndSubpackages() {
        const data = await ApplicationState.cacheFetch(ApplicationState.GROUPS_URL);
        return data.groups;
    }
}

// Convenience export
export async function cacheFetch(url, options) {
    return ApplicationState.cacheFetch(url, options);
}
