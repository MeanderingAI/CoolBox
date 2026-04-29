# Go Migration Docs Reorganization

## Summary
- Moved the detailed Go narrow C ABI roadmap out of the internal documentation area and into the versioned `plan/` directory.
- Split the roadmap into dedicated Markdown files for each migration module.

## Files Updated
- `docs/internal_documents/extensions/go/README.md`

## Files Added
- `plan/v1.1.40_v1.1.41/go_narrow_c_abi_migration_overview.md`
- `plan/v1.1.40_v1.1.41/go_linear_regression_migration.md`
- `plan/v1.1.40_v1.1.41/go_decision_tree_migration.md`
- `plan/v1.1.40_v1.1.41/go_hmm_migration.md`
- `plan/v1.1.40_v1.1.41/go_multi_arm_bandit_migration.md`
- `plan/v1.1.40_v1.1.41/go_pca_migration.md`
- `plan/v1.1.40_v1.1.41/go_svm_migration.md`
- `plan/v1.1.40_v1.1.41/go_graphics_charting_migration.md`
- `plan/v1.1.40_v1.1.41/go_gui_components_migration.md`
- `plan/v1.1.40_v1.1.41/go_migration_docs_reorganization.md`
- `plan/v1.1.40_v1.1.41/validation_results.md`

## Files Removed
- `docs/internal_documents/extensions/go/narrow_c_abi_migration.md`

## Result
- The Go internal README now points at the plan-based overview and per-module migration documents.
- Each migration module now has its own dedicated Markdown file instead of sharing one large roadmap document.
