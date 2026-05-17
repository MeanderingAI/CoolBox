# v1.3.4 addendum: Taylor expansion components demo

Status
- Implemented and validated in the current workspace.
- Added a new Taylor convergence demo with a slider-driven component count.

Summary
- Added a dedicated demo focused on Taylor series partial sums rather than signal analysis.
- The demo lets users slide the number of Taylor components and compare the exact curve against the selected approximation.
- Earlier partial sums are also rendered so the stepwise convergence is visible.
- Added PNG export for the current chart view.
- Added GIF sweep export that animates the component slider through the convergence path.

Files added
- _internal_workspace/demo_workspaces/taylor_expansion_components_demo/index.html
- _internal_workspace/demo_workspaces/taylor_expansion_components_demo/demo.json
- _internal_workspace/demo_workspaces/taylor_expansion_components_demo/README.md

Behavior changes
- The visualization canvas fills the available panel height.
- The component slider updates the Taylor partial sum in real time.
- Function selection supports sin(x), cos(x), and e^x.
- The chart can be exported as a PNG image.
- The chart can be exported as an animated GIF sweep.

Validation executed
- File syntax check passed for the new demo HTML.
- No browser run was performed in this session.

Release notes bullets for v1.3.4
- Added a new Taylor expansion convergence demo with a slider for the number of components.
- Rendered earlier partial sums so the convergence path is visible.
- Added PNG export, GIF sweep export, and full-height canvas behavior to the demo.

Follow-up candidates (post v1.3.4)
- Add a launch/task entry for quick access to the new demo.
- Add a more advanced mode that animates one term at a time.
- Add per-term contribution labels or tooltips for classroom use.
