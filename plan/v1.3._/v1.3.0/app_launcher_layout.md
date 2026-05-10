# App Launcher — button layout fix

**File:** `GUI/static/screens/package_builder/app-launcher.mjs`  
**Date:** May 9 2026  
**Status:** complete

## Problem

`.ac-target-row` was `display:flex` with default `flex-direction:row`, causing the target name, build/launch/run buttons, and status badge to compete horizontally. At normal panel widths the buttons were too cramped to read.

## Fix

- `.ac-target-row` changed to `flex-direction:column; gap:0.35em` so name and actions stack vertically
- Removed `flex:1; min-width` from `.ac-target-name`
- Added `.ac-target-actions { display:flex; align-items:center; gap:0.4em; flex-wrap:wrap; }` as a dedicated button row
- Both `_makeCmakeRow()` and `_makePythonRow()` now create an `actions` div, append all buttons and the status span into it, then do `row.append(name, actions)`
