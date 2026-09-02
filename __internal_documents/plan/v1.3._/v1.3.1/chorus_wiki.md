# Chorus — Wiki

## Overview

A Confluence/Notion-style team wiki added as a client front-end at  
`business_suite/client_fe/chorus/index.html`.

Self-contained single HTML file — no build step, no external dependencies.  
Grouped under the **Internal Tooling** section of the Client FE Hub.

Brand accent colour: `#16a34a` (green).

---

## Layout

```
┌─ Fixed Topbar (brand, search, avatar) ────────────────────────────┐
│                                                                   │
│  ┌─ Sidebar (260px) ──┐  ┌─ Article Area ──────┐  ┌─ TOC Rail ─┐ │
│  │ + New Page         │  │ Breadcrumb           │  │ On page    │ │
│  │                    │  │ Meta (space/date)    │  │ ─ heading  │ │
│  │ SPACES             │  │ <h1> Title           │  │ ─ heading  │ │
│  │ · Engineering      │  │ Deck paragraph       │  │            │ │
│  │ · Product          │  │ ── Toolbar ──        │  │ Contributors│ │
│  │ · Design           │  │ [Edit][Share][Watch] │  │ Tags       │ │
│  │ · HR & People      │  │                      │  └────────────┘ │
│  │ · General          │  │ Rich article body:   │                 │
│  │                    │  │  h2 / h3 headings    │                 │
│  │ IN THIS SPACE      │  │  paragraphs          │                 │
│  │ · Overview         │  │  ul / ol lists       │                 │
│  │ · Getting Started  │  │  code blocks         │                 │
│  │ · Architecture     │  │  blockquotes         │                 │
│  │ · API Reference    │  │  callout boxes       │                 │
│  │ · Contributing     │  │  data tables         │                 │
│  │ · Changelog        │  └──────────────────────┘                 │
│  └────────────────────┘                                           │
└───────────────────────────────────────────────────────────────────┘
```

---

## Features

### Sidebar
- **Spaces list** — 5 colour-coded spaces (Engineering, Product, Design, HR & People, General) with page counts
- **Page tree** — per-space page list; clicking switches the article content and highlights the active item
- **+ New Page** button — opens creation modal
- Back link to the Client FE Hub

### Article Area
- **Breadcrumb** — `Spaces › Space Name › Page Title`
- **Meta row** — space name, last-modified date, author, tag badge
- **Inline edit mode** — toggled by the Edit button; sets `contenteditable="true"` on the title and body; Save button commits (mock)
- **Toolbar** — Edit, Share, Watch, Export, History

### Rich Article Content
- `<h2>` / `<h3>` section headings
- Paragraphs, ordered and unordered lists
- Fenced code blocks (`background: #0f172a`)
- Blockquotes with left brand-coloured border
- Callout boxes (default / info / warning variants)
- Data tables with hover highlight

### Right Rail
- **TOC** — jump links to each `<h2>` in the article, active-state highlighted
- **Contributors** list
- **Tags** pill display

### New Page Modal
- Fields: title, space selector, template selector
- Templates: Blank, How-to guide, Meeting notes, ADR, Runbook, API Reference
- On create: appends new item to the sidebar page tree

### Search
- Search input in topbar; fires toast on input

---

## Default Content

The default page shown is **Engineering Wiki — Overview** containing:
- Quick Links
- Architecture section with an info callout
- Tech Stack table (Frontend / API Gateway / Services / Database / Cache / Build)
- Contributing guidelines
- Example code snippet (JSON API request)
- Blockquote
- Related Spaces section

---

## Design Language

| Token | Value |
|-------|-------|
| Brand accent | `#16a34a` (green) |
| Sidebar width | `260px` |
| Topbar height | `56px` |
| Brand icon | Green→sky gradient, `📖` |
| Font | `system-ui, -apple-system, 'Segoe UI', sans-serif` |

Shares the global Client FE design tokens (`--text`, `--muted`, `--border`, `--surface`, `--white`).

---

## File Location

```
business_suite/client_fe/chorus/index.html
```
