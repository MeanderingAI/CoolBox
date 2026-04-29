# Go Narrow C ABI Refactor Plan

## Summary
- Added a concrete maintainer-facing refactor plan for narrowing the Go bindings onto a stable C ABI.
- Documented the first recommended migration slice as linear regression, including a proposed minimal ABI shape and migration steps.

## Files Added
- `docs/internal_documents/extensions/go/narrow_c_abi_migration.md`
- `plan/v1.1.40_v1.1.41/go_narrow_c_abi_refactor_plan.md`
- `plan/v1.1.40_v1.1.41/validation_results.md`

## Files Updated
- `docs/internal_documents/extensions/go/README.md`

## Result
- The Go internal documentation now includes a concrete path for moving from the current broad bridge toward a smaller supported C ABI.
- The refactor plan is anchored to the repository's existing bridge files rather than a hypothetical new design.
- The first ABI sketch is scoped tightly enough to guide an incremental implementation starting with linear regression.