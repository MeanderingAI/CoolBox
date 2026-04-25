- Replaced the LSP-specific GHCR login fragment with a generic ghcr-login.yaml (includes owner normalization) for all LSP workflows.
---

**[v1.1.73] Modularize LSP workflow steps**

- Extracted common LSP dependency install and GHCR login steps into reusable fragments:
	- .github/workflows/fragments/lsp-install-deps.yaml
	- .github/workflows/fragments/lsp-ghcr-login.yaml
- Updated all LSP workflow YAMLs (lsp-java, lsp-python, lsp-rust, lsp-vlang, lsp-c3) to use these fragments for DRY and maintainable CI.
putting documentation related yamls in specific folder

seperating generate_purchase yamls, which are dependent on the purchase pipeline.

---

**[v1.1.73] Fix msys2: command not found in GitHub Actions**

- Restored and uncommented the MSYS2 install step (`uses: msys2/setup-msys2@v2`) for both x86_64 and ARM64 before any step using `shell: msys2 {0}`.
- Ensured all pacman and msys2 shell steps are always preceded by the MSYS2 setup step.
- This prevents 'msys2: command not found' errors in Windows CI jobs.

---

**[v1.1.73] Modularize generate_purchase workflow steps**

- Extracted common steps from all generate_purchase workflows (C, Python, JS, Rust) into reusable fragments:
    - .github/workflows/fragments/purchase-checkout.yaml
    - .github/workflows/fragments/purchase-compute-artifact-name.yaml
    - .github/workflows/fragments/purchase-install-native-build-tooling.yaml
    - .github/workflows/fragments/purchase-configure-c-bindings.yaml
    - .github/workflows/fragments/purchase-build-c-bindings.yaml
    - .github/workflows/fragments/purchase-list-configured-tests.yaml
    - .github/workflows/fragments/purchase-package-c-bindings-tests.yaml
- Updated all generate_purchase YAMLs to use these fragments for checkout, artifact name computation, and (for C) install, configure, build, test listing, and packaging steps.
- This ensures DRY, maintainable, and consistent CI for all generate_purchase workflows.