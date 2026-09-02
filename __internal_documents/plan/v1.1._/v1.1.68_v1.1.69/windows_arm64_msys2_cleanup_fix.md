
# Patch: Fix MSYS2 ARM64 Install Error (v1.1.67 → v1.1.68)

## Context
GitHub Actions for Windows ARM64 failed with:

```
Error: Trying to install MSYS2 to D:\a\_temp\msys64 but that already exists, cannot continue.
```

This happens because a stale MSYS2 directory is not removed before running `msys2/setup-msys2@v2` for ARM64 jobs.

## Change
- Updated `.github/workflows/build-libs.yaml`:
	- Added a dedicated cleanup step for ARM64 (`windows-arm64`) to remove `D:\a\_temp\msys64` before MSYS2 setup.
	- The existing cleanup step for all Windows jobs now excludes ARM64 to avoid duplicate steps.

## Patch
```yaml
-      - name: Clean up stale MSYS2 install
-        if: runner.os == 'Windows'
+      - name: Clean up stale MSYS2 install (all Windows)
+        if: runner.os == 'Windows' && matrix.platform != 'windows-arm64'
				 run: |
					 if (Test-Path 'D:\a\_temp\msys64') { Remove-Item -Recurse -Force 'D:\a\_temp\msys64' }
				 shell: pwsh
+
+      - name: Clean up stale MSYS2 install (ARM64)
+        if: runner.os == 'Windows' && matrix.platform == 'windows-arm64'
+        run: |
+          if (Test-Path 'D:\a\_temp\msys64') { Remove-Item -Recurse -Force 'D:\a\_temp\msys64' }
+        shell: pwsh
```

## Rationale
This ensures the MSYS2 directory is always removed before setup, preventing install errors on ARM64 jobs.

## Version
- From: v1.1.67
- To:   v1.1.68

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
