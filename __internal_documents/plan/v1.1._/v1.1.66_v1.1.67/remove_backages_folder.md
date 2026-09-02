# Remove _libraries/backages after test migration

All ML test files and folders have been moved from `_libraries/backages` to `_libraries/packages`. The `_libraries/backages` folder is now removed for clarity and to avoid confusion. All CMakeLists.txt files should reference the new locations under packages.

**Date:** 2026-04-19
**Author:** GitHub Copilot
