# Client FE Hub

## Overview

A landing/hub page at `business_suite/client_fe/index.html` that organises all client front-end applications into three audience-based sections.

Replaces an earlier flat grouped layout (5 groups). The current structure uses three top-level **sections** with full-width rule dividers, each containing an app grid of cards.

---

## Section Structure

### Section 1 — External Facing
*Apps your clients and prospects interact with directly.*

| App | Path | Badge |
|-----|------|-------|
| Client Portal | `client_facing_portal/index.html` | Portal |
| Contracts | `contracts/index.html` | Legal |
| Office Sweet · Drive | `office_sweet/index.html` | Drive |

Accent colour: `#2563eb` (blue)

---

### Section 2 — Internal Tooling
*Tools used day-to-day by your team.*

| App | Path | Badge |
|-----|------|-------|
| Clients | `clients/index.html` | CRM |
| Chorus · Wiki | `chorus/index.html` | New |
| Mermaid · Diagrams | `mermaid/index.html` | New |

Accent colour: `#7c3aed` (violet)

---

### Section 3 — Employee Pages
*Self-service HR and authentication pages for staff.*

| App | Path | Badge |
|-----|------|-------|
| Internal Login | `internal_login/index.html` | Auth |
| Timesheets | `timesheets/index.html` | HR |

Accent colour: `#059669` (emerald)

---

## Section Header Design

Each section uses a `.section-rule` row:

```
● (coloured dot)  Section Title  — subtitle text  ─────────────────  N apps
```

- Dot: 13px coloured circle
- Title: `font-size: 1.05rem; font-weight: 900`
- Subtitle: muted description of the audience
- Rule line: `flex: 1; height: 1.5px; background: var(--border)` — pushes count to the right
- Count: pill badge (same style as group counts)

---

## App Card Design

Each app is a `<a class="app-card">` link with:

- **Top row**: icon (46×46 px coloured square, rounded 12px, emoji) + badge pill (type/audience label)
- **Name**: `font-size: 1rem; font-weight: 800`
- **Description**: 1–2 sentence summary
- **Footer**: "Open ›" link + path tag, separated by a top border

Hover effect: `border-color: var(--brand)`, `box-shadow`, `translateY(-2px)`.

---

## Badge Colour Map

| Badge class | Use case | Colours |
|-------------|----------|---------|
| `badge-external` | External-facing apps | Blue `#2563eb` |
| `badge-internal` | Internal tools | Violet `#7c3aed` |
| `badge-employee` | Employee HR pages | Emerald `#059669` |
| `badge-office` | Office Sweet Drive | Sky `#0284c7` |
| `badge-legal` | Contracts | Amber `#b45309` |
| `badge-auth` | Login / Auth | Pink `#be185d` |
| `badge-new` | New apps (Chorus, Mermaid) | Green `#16a34a` |

---

## Design Language

| Token | Value |
|-------|-------|
| Navbar background | `#0f172a` |
| Brand | `Meandering LLC` with `<span>` accent `#60a5fa` |
| Page max-width | `1080px` |
| Card border-radius | `14px` |
| Grid | `auto-fill, minmax(240px, 1fr)` |
| Font | `system-ui, -apple-system, 'Segoe UI', sans-serif` |
| Avatar | 30×30 px initials `SC` |
| MOCK badge | `background: #fef3c7; border: 1px solid #fde68a; color: #92400e` |

---

## History

| Version | Change |
|---------|--------|
| Initial | Single flat hub with 5 named groups: Client Facing Portal, Core Business Tools, Office Sweet, Chorus, Mermaid |
| v1.3.1 | Reorganised into 3 audience sections: External Facing, Internal Tooling, Employee Pages. Apps redistributed accordingly. |

---

## File Location

```
business_suite/client_fe/index.html
```
