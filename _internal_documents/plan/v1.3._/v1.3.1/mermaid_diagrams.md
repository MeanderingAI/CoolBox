# Mermaid — Diagrams

## Overview

An interactive SVG-based diagram drawing tool added as a client front-end at  
`business_suite/client_fe/mermaid/index.html`.

Self-contained single HTML file — no build step, no external dependencies, no third-party diagramming library.  
All rendering is done via native SVG DOM manipulation in vanilla JS.  
Grouped under the **Internal Tooling** section of the Client FE Hub.

Brand accent colour: `#f59e0b` (amber). Dark canvas theme throughout.

---

## Layout

```
┌─ Fixed Topbar ────────────────────────────────────────────────────┐
│  Brand · Doc name (editable) · Tool group · [Export][Share][SC]   │
├─ Left Panel (220px) ─┬─ Canvas ──────────────────────┬─ Right Panel (240px) ─┤
│ Shapes palette       │  · · · · · · · · · · · · · ·  │ Properties           │
│  Process / Decision  │  ┌──────────────────────────┐  │                      │
│  Terminal / I/O      │  │ Canvas toolbar            │  │ (No selection)       │
│  Database / Cloud    │  │ [Flow][Seq][Org][Arch]    │  │  or                  │
│                      │  │ [Fit][Clear]              │  │ Label                │
│ Diagram Type         │  └──────────────────────────┘  │ Shape                │
│  Flowchart / Seq     │                                 │ Fill colour          │
│  Org Chart / Arch    │       SVG diagram              │ Position (X, Y)      │
│                      │                                 │ Size (W, H)          │
│ My Diagrams          │                         [zoom]  │ Duplicate / Delete   │
│  System Architecture │                                 │                      │
│  Onboarding Flow     │                                 │                      │
│  Auth Sequence       │                                 │                      │
│  Org Chart Q2        │                                 │                      │
└──────────────────────┴─────────────────────────────────┴──────────────────────┘
```

---

## Features

### Canvas
- SVG element fills the canvas area
- Dot-grid background (`radial-gradient` pattern at 24px spacing)
- Nodes rendered as SVG `<rect>`, `<polygon>` (diamond), or `<ellipse>` (oval)
- Edges rendered as SVG `<path>` quadratic bezier curves with `marker-end` arrowheads
- Edge labels rendered as SVG `<text>` above the curve midpoint
- Click a node to select it and populate the properties panel

### Toolbar (topbar)
| Tool | Key | Description |
|------|-----|-------------|
| Select | V | Click to select nodes |
| Pan | H | Hand tool (planned) |
| Rectangle | R | Add rectangle to canvas |
| Decision | D | Add diamond to canvas |
| Oval | O | Add oval to canvas |
| Connect | C | Draw connector between nodes |
| Text | T | Add text label |

### Shape Palette (left panel)
Clicking a shape button appends a new node of that type to the current diagram data and re-renders:
- Process (rect), Decision (diamond), Terminal (oval), I/O (parallelogram), Database (cylinder), Cloud

### Diagram Type buttons (left panel)
Load a preset template: Flowchart, Sequence, Org Chart, Architecture.

### Canvas Toolbar (overlay)
- Tab strip: Flowchart / Sequence / Org Chart / Architecture — switches the rendered diagram
- **Fit** — resets zoom to 100% and pan to origin
- **Clear** — removes all nodes and edges from the current diagram (with confirmation)

### Zoom
- `+` / `−` buttons: step ±10%, range 30%–300%
- Mouse wheel: continuous zoom ±8% per tick
- Zoom level displayed as percentage

### Properties Panel (right panel)
Shown when a node is selected:
- **Label** — live-updates node text on input
- **Shape** — dropdown (Rectangle / Diamond / Oval)
- **Fill colour** — 6 colour swatches
- **Position** — X, Y inputs (re-renders on change)
- **Size** — W, H inputs (re-renders on change)
- **Duplicate** — clones node offset by 24px
- **Delete** — removes node and all its connected edges

### Document Name
- Editable inline input in the topbar
- Shows `● Unsaved` while typing, `● Saved` on blur

---

## Built-in Diagrams

| Key | Title | Nodes | Edges |
|-----|-------|-------|-------|
| `arch` | System Architecture | 8 (ovals, rects, cylinders, cloud) | 7 |
| `flow` | Onboarding Flow | 6 (ovals, rects, diamond) | 6 |
| `seq` | Auth Sequence | 4 (rects — actors) | 6 |
| `org` | Org Chart Q2 | 8 (rects) | 7 |

All diagram data is stored in a plain JS object (`DIAGRAMS`) — mutations (add/delete/move) persist for the session.

---

## Node Shape Rendering

| Type key | SVG element | Stroke colour |
|----------|-------------|---------------|
| `rect` | `<rect rx="8">` | `#2563eb` (blue) |
| `diamond` | `<polygon>` | `#f59e0b` (amber) |
| `oval` | `<ellipse>` | `#8b5cf6` (violet) |
| `cylinder` | `<rect rx="8">` | `#16a34a` (green) |
| `cloud` | `<rect rx="20">` | `#f59e0b` (amber) |

---

## Design Language

| Token | Value |
|-------|-------|
| Brand accent | `#f59e0b` (amber) |
| Canvas background | `#0f172a` with dot-grid |
| Panel background | `#0f172a` |
| Panel border | `#1e293b` |
| Node text | `#e2e8f0` |
| Edge stroke | `#334155` |
| Font | `system-ui, -apple-system, 'Segoe UI', sans-serif` |

---

## File Location

```
business_suite/client_fe/mermaid/index.html
```
