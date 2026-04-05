# Go Extracted Module Tests And Purchase Workflow Update

## Summary

Go-side tests were expanded to cover the extracted wrapper modules, and the Go purchase workflow was updated to preserve verbose test logs as CI artifacts.

## Changes

- added `_libraries/go_bindings/extracted_modules_test.go`
- added smoke coverage for extracted modules including HMM, PCA, SVM, multi-arm bandit, Bayesian network, GUI constructors, and graphics guard paths
- updated `.github/workflows/generate-purchase-go.yaml` to run `go test` with `-count=1 -v`
- added upload of a per-platform Go test log artifact from the purchase workflow

## Rationale

The wrapper-level ABI migration significantly changed the Go file layout. The added tests exercise the extracted surfaces directly so regressions are easier to catch after future wrapper or native-side refactors. The workflow change makes failures diagnosable from CI artifacts instead of relying only on console logs.

## Validation

- editor diagnostics reported no issues in the new test file or updated workflow
- runtime execution was not validated locally in this environment because terminal-based execution remains unreliable

## Next Steps

- add focused native-side validation once the bridge implementation starts being split by module
- optionally convert the Go workflow to emit a machine-readable test report in addition to the verbose log artifact
