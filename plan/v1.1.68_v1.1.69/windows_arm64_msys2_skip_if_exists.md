
# Patch: Skip MSYS2 Setup if Already Installed (ARM64)

## Context
The ARM64 workflow failed if `D:\a\_temp\msys64` already existed. Instead of deleting the directory, the workflow now checks for its existence and skips the MSYS2 setup step if present.

## Change
- Updated `.github/workflows/build-libs.yaml`:
	- Added a step to skip MSYS2 setup for ARM64 if `msys64` already exists.
	- The MSYS2 setup step for ARM64 now only runs if the directory does not exist.

## Patch
```yaml
-      - name: Clean up stale MSYS2 install (ARM64)
-        if: runner.os == 'Windows' && matrix.platform == 'windows-arm64'
-        run: |
-          if (Test-Path 'D:\a\_temp\msys64') { Remove-Item -Recurse -Force 'D:\a\_temp\msys64' }
-        shell: pwsh
+
+      - name: Skip MSYS2 setup if already installed (ARM64)
+        if: runner.os == 'Windows' && matrix.platform == 'windows-arm64' && (exists('D:\a\_temp\msys64'))
+        run: |
+          echo "MSYS2 already installed at D:\a\_temp\msys64, skipping setup."
+        shell: pwsh
+
+      - name: Install MSYS2 and MinGW for ARM64
+        if: runner.os == 'Windows' && matrix.platform == 'windows-arm64' && !(exists('D:\a\_temp\msys64'))
+        uses: msys2/setup-msys2@v2
+        with:
+          msystem: MINGW64
+          path-type: inherit
+          update: true
```

## Rationale
This avoids unnecessary deletion and prevents install errors if MSYS2 is already present.

## Version
- From: v1.1.67
- To:   v1.1.68

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
