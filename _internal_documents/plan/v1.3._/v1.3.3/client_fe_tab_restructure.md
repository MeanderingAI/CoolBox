# Client FE Viewer — Tab Restructure — v1.3.3

## Summary
Restructured the **Client FE** sub-tab inside Package Builder. Added an **Employee Chart** tab for internal HR portals, added and then removed an **Internal Tooling** tab (Chorus / Mermaid / Office Suite), and updated all folder path labels to show full project-root relative paths.

## Final Tab Structure
| Tab | Contents |
|-----|----------|
| 🌐 Portals | Public-facing portals only (e.g. `client_facing_portal`) |
| 🔧 Middleware | Middleware tools from `_interfaces/business_suite/middle_wear/` |
| 👥 Employee Chart | Contracts, Clients, Internal Login, Timesheets |

## File Changed: `_interfaces/GUI/static/screens/package_builder/client-fe-viewer.mjs`

### Employee Chart tab
- Added `EMPLOYEE_PORTALS = new Set(['contracts', 'clients', 'internal_login', 'timesheets'])`
- `_load()` now splits the `/client-fe` response into `publicPortals` (not in the set) and `employeePortals` (in the set)
- Public portals render in the existing Portals grid; employee portals render in a dedicated **Employee Chart** grid via `_populateEmployeeGrid()`
- `👥 Employee Chart` inner-tab button and section added to the component
- `_populateEmployeeGrid()` method added — re-uses `_makeCard()` so behaviour is identical to the Portals tab

### Internal Tooling tab (added then removed)
- A `🛠 Internal Tooling` tab was briefly added with cards for Chorus, Mermaid, and Office Suite (external URL + optional embed)
- Removed at user request; the tab, section, `INTERNAL_TOOLS` array, `_makeInternalToolsPanel()` method, and all associated CSS were deleted

### Full path labels
- Portal card subtitle changed from `client_fe/{folder}` → `_interfaces/business_suite/client_fe/{folder}`

## File Changed: `_interfaces/GUI/static/screens/package_builder/middle-wear-viewer.mjs`

- Middleware card folder label changed from `{folder}` → `_interfaces/business_suite/middle_wear/{folder}`
