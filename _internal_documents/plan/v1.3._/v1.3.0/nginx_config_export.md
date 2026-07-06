# Nginx Setup — raw config export

**File:** `business_suite/middle_wear/nginx_setup/index.html`  
**Date:** May 9 2026  
**Status:** complete

## What was added

- Replaced the static placeholder in the Raw Config tab with a live-generated nginx.conf panel
- Sidebar vhost click updates the displayed config in real time via `cfgRender()`
- Tab-bar click listener re-triggers `cfgRender()` when the Raw Config tab is activated

## Config generation (`cfgGenerate`)

Produces a full nginx server block per vhost:

- `upstream` group with all backend servers
- HTTP (port 80) server block that redirects to HTTPS
- HTTPS (port 443) server block with:
  - SSL certificate paths
  - `gzip` compression settings
  - Security headers (HSTS, X-Frame-Options, X-Content-Type-Options, etc.)
  - `proxy_pass` to upstream (for reverse-proxy vhosts) or `root` + `try_files` (for static vhosts)

## Syntax highlighting (`cfgHighlight`)

Colours nginx keywords, quoted strings, port numbers, and `#` comments.

## Export

- Copy button copies raw config text to clipboard
- Download button saves as `<hostname>.conf`
