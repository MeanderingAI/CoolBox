# v1.3.4 addendum: Taylor Series and signal processing demo

Status
- Implemented in the current workspace.
- Demo UI updated with PNG export and a full-height visualization panel.

Summary
- Added a dedicated Taylor series and signal processing demo under the internal demo workspace.
- Extended the interactive canvas workflow so the visualization fills the available vertical space in the demo layout.
- Added one-click PNG export for the current canvas view in addition to the existing CSV data export.

Files added or updated
- _internal_workspace/demo_workspaces/taylor_series_demo/index.html
- _internal_workspace/demo_workspaces/taylor_series_demo/README.md
- _internal_workspace/demo_workspaces/taylor_series_demo/demo.json

Behavior changes
- The visualization panel now expands to the available screen height instead of using a fixed-height canvas.
- The demo now includes an Export PNG action that captures the current chart state.
- Existing Export Data behavior remains available for CSV output.

Validation status
- Change was implemented in the workspace.
- No separate build or browser validation was run as part of this documentation update.

Release notes bullets for v1.3.4
- Added a Taylor series and signal processing demo in the internal demo workspace.
- Added PNG export for the demo canvas.
- Updated the demo layout so the visualization fills the available vertical space.
- Preserved CSV export for the sampled analysis data.

Follow-up candidates (post v1.3.4)
- Wire the demo into a task or launch entry for quicker access from VS Code.
- Add a dedicated export control for the currently selected tab or plot mode.
- Consider a responsive two-pane fallback for narrow screens.
