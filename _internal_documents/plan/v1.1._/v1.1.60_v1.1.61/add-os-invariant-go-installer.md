# Add OS-invariant Go installer script for local development

## Problem
Go is required to build and test the Go bindings, but is not always installed or available in the developer's environment. Manual installation is error-prone and platform-specific.

## Solution
A cross-platform Go installer script was added:

- **Script:** `_scripts/install_go.py`
- **Features:**
  - Detects Windows, macOS, or Linux
  - Downloads the correct Go binary distribution (v1.22.3)
  - Extracts to `_local_build_pipeline/tmp/go` for workspace-local use
  - Prepends the Go binary directory to PATH for the current process
  - Verifies installation with `go version`
- **Usage:**
  - Run with: `python _scripts/install_go.py`
  - Go will be available for the current session and can be used to build/test Go bindings

## Rationale
This ensures all developers and CI environments can reliably install and use Go, regardless of OS, without requiring admin rights or system-level changes.

## Status
- [x] Script created and tested on Windows
- [ ] Test on Linux/macOS (pending)
- [x] Documented in this plan

---

**Documented by GitHub Copilot, 2026-04-18**
