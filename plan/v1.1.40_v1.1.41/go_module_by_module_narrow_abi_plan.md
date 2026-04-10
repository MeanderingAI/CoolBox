# Go Module-By-Module Narrow ABI Plan

## Summary
- Expanded the Go narrow C ABI migration document to include an explicit module-by-module roadmap.
- Clarified the earlier "slice" wording by describing the migration units as feature modules with their own ABI, Go wrapper, and native implementation targets.

## Files Updated
- `docs/internal_documents/extensions/go/narrow_c_abi_migration.md`
- `docs/internal_documents/extensions/go/README.md`

## Result
- The Go migration plan now includes dedicated sections for:
  - linear regression
  - decision tree
  - HMM
  - multi-arm bandit
  - PCA
  - SVM
  - graphics and charting
  - GUI components
- Each module now has a clearer migration target, file layout expectation, and sequencing note.