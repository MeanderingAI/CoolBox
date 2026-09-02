v1.3.4 addendum: middleware routing and UUID generator stabilization

Status
- Implemented and validated in current workspace.
- Middleware entries now source directly from _interfaces/business_suite/middle_wear.
- UUID generator is extension-first and extension-only (no local UUID fallback engine).

Scope summary
- Corrected middle-portal routing so middleware pages resolve from the real _interfaces path.
- Updated Client FE middleware tab links to point directly to middle_wear pages.
- Removed duplicate client_fe middleware wrapper directory to avoid path shadowing.
- Added UUID Emscripten asset serving and build endpoints in GUI backend.
- Updated UUID generator UI to check assets, expose in-page asset build action, and show build progress dialog.

Problem addressed
- Users encountered: {"error":"index.html not found for this tool."}
- Users encountered: [ERR] Failed to load uuid_generation.js
- Root causes:
  - middle-portal route pointed at business_suite/middle_wear instead of _interfaces/business_suite/middle_wear.
  - Relative link resolution and duplicate wrapper layout caused ambiguous/misleading middleware paths.
  - UUID extension assets (uuid_generation.js/.wasm) were not always present in build output and no in-page remediation path existed.

Files touched
- _interfaces/GUI/main.py
- _interfaces/business_suite/client_fe/index.html
- _interfaces/business_suite/middle_wear/uuid_generator/index.html
- _interfaces/GUI/static/screens/package_builder/middle-wear-viewer.mjs

Routing and middleware integration updates

Server route fix
- Endpoint: /middle-portal/{folder}
- Updated route base from:
  - business_suite/middle_wear
- To:
  - _interfaces/business_suite/middle_wear
- Result: /middle-portal/database_management now resolves to:
  - _interfaces/business_suite/middle_wear/database_management/index.html

UUID asset routes
- Added asset serving endpoint:
  - /middle-portal-assets/uuid_generation/{asset_name}
- Added asset build endpoint:
  - /middle-portal-assets/uuid_generation/build
- Build endpoint invokes uuid_generation_js target build and reports js/wasm existence in response.

Client FE middleware tab changes
- Middleware section lists and links directly to canonical middle_wear tools:
  - /_interfaces/business_suite/middle_wear/uuid_generator/index.html
  - /_interfaces/business_suite/middle_wear/nginx_setup/index.html
  - /_interfaces/business_suite/middle_wear/database_management/index.html
  - /_interfaces/business_suite/middle_wear/distributed_setup/index.html
- Removed duplicate source under:
  - _interfaces/business_suite/client_fe/middleware

UUID generator tool changes

Design and UX
- Reduced tool to requested minimal interaction model:
  - UUID version select
  - Batch size input
  - Generate button
  - Output pane (plus copy/clear helpers)
- Added in-page Generate Assets button and progress dialog when extension assets are missing.

Runtime behavior
- Extension-only behavior:
  - Uses createUuidGenerationModule and exported uuid_v1..uuid_v8/guid functions.
  - No local fallback UUID engine.
- Asset pre-check behavior:
  - Verifies uuid_generation.js and uuid_generation.wasm availability before module load.
  - Shows actionable build path in-page when assets are missing.

Generation capabilities
- Supported options in tool:
  - v1, v2, v3, v4, v5, v6, v7, v8, guid
- Batch generation:
  - configurable count (1..1000)
  - one identifier per line in output

Validation notes
- Diagnostics clean for updated files.
- Middleware directories verified to contain index.html at canonical locations.
- middle-portal route aligned with list_middle_wear directory source.
- UUID C++ package and Emscripten bindings updated to expose v7 and tested.

Release notes bullets for v1.3.4
- Fixed middleware portal routing to correctly serve _interfaces/business_suite/middle_wear tools.
- Standardized Client FE middleware tab to canonical middle_wear paths.
- Removed duplicate client_fe middleware wrappers that caused path confusion.
- Added UUID extension asset serving/build endpoints and integrated extension asset generation into middleware UI.
- Reworked UUID generator to extension-only runtime with in-page asset checks and build-progress dialog.
