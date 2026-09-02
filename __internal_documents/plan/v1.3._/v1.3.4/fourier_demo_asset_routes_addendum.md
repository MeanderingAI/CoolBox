# v1.3.4 addendum: Fourier demo asset routing and build endpoints

Status
- Implemented in current workspace.
- Extended GUI backend routing for demo-local assets and Fourier module artifacts.

Summary
- Added dedicated endpoints to serve and build Fourier Emscripten artifacts for the Fourier demo.
- Added a generic demo asset endpoint so scripts and related files under demo_workspaces are served reliably.
- This resolves relative asset loading failures when opening demos through /demo/{folder}.

Files updated
- _interfaces/GUI/main.py

New endpoints
- GET /demo-assets/fourier_fft_demo/module/{asset_name}
  - Serves fourier_tranforms.js and fourier_tranforms.wasm from build output.
- POST /demo-assets/fourier_fft_demo/module/build
  - Builds fourier_tranforms_js and returns status plus artifact presence.
- GET /demo-assets/{folder}/{asset_path:path}
  - Serves assets from _internal_workspace/demo_workspaces/{folder}/.

Behavior changes
- Demo JS now loads through stable backend-hosted paths.
- Fourier module assets can be generated on demand from the dashboard workflow.
- Missing asset conditions return actionable errors tied to the expected build target.

Validation executed
- File diagnostics passed for backend route additions.
- Endpoint behavior was validated by code inspection; no live server run was executed in this session.

Release notes bullets for v1.3.4
- Added Fourier demo module serve/build endpoints under /demo-assets/fourier_fft_demo/module/.
- Added generic /demo-assets/{folder}/{asset_path:path} routing for demo workspace assets.
- Improved dashboard demo loading reliability by removing dependence on fragile relative paths.
