# Office Sweet — Drive

## Overview

A Google Drive-style cloud file storage UI added as a client front-end at  
`business_suite/client_fe/office_sweet/index.html`.

Self-contained single HTML file — no build step, no external dependencies.  
Grouped under the **External Facing** section of the Client FE Hub.

---

## Layout

```
┌─ Fixed Topbar (search pill, avatar) ──────────────────────────────┐
│                                                                   │
│  ┌─ Sidebar ──────┐  ┌─ Main Content ─────────────────────────┐  │
│  │ My Drive       │  │ Recent Strip (horizontal scroll)        │  │
│  │ Shared with me │  │                                         │  │
│  │ Recent         │  │ File Grid / List view                   │  │
│  │ Starred        │  │                                         │  │
│  │ ─────────────  │  │                                         │  │
│  │ Docs           │  │                                         │  │
│  │ Sheets         │  │                                         │  │
│  │ Slides         │  │                                         │  │
│  │ Trash          │  │                                         │  │
│  │ ─────────────  │  │                                         │  │
│  │ Storage meter  │  │                                         │  │
│  │ 3.8 / 10 GB    │  │                                         │  │
│  └────────────────┘  └─────────────────────────────────────────┘  │
└───────────────────────────────────────────────────────────────────┘
```

---

## Features

### Sidebar
- Collapsible navigation: My Drive, Shared with me, Recent, Starred, and type filters (Docs, Sheets, Slides, Trash)
- Storage usage meter showing 3.8 GB / 10 GB

### File Views
- **Grid view** — file cards with emoji thumbnails and per-type background tints
- **List view** — tabular rows with name, owner, modified date, size
- Toggle between views with a toolbar button

### Recent Strip
- Horizontal scroll strip of recently accessed files above the main grid

### File Cards
- Emoji thumbnail with type-based background colour (folder/doc/sheet/slide/pdf/img/vid)
- Star toggle (⭐)
- Per-card context menu (⋮) with rename, download, share, move to trash options
- 16 mock files across 7 types

### "+ New" Modal
- Triggered by the "+ New" button
- 6 creation options: Folder, Document, Spreadsheet, Presentation, Form, Upload

### Other Interactions
- Drag-and-drop upload zone
- Search bar with sort and filter controls
- Toast notifications for all actions
- Keyboard `Escape` closes modals

---

## Design Language

Consistent with all other Client FE pages:

| Token | Value |
|-------|-------|
| Navbar background | `#0f172a` |
| Brand | `Meandering LLC` with `<span>` accent `#60a5fa` |
| `--brand` | `#2563eb` |
| `--text` | `#1e293b` |
| `--muted` | `#64748b` |
| `--border` | `#e2e8f0` |
| `--surface` | `#f8fafc` |
| Font | `system-ui, -apple-system, 'Segoe UI', sans-serif` |
| Avatar | 30×30 px, initials `SC` |
| MOCK badge | `background: #fef3c7; border: 1px solid #fde68a; color: #92400e` |

---

## File Location

```
business_suite/client_fe/office_sweet/index.html
```
