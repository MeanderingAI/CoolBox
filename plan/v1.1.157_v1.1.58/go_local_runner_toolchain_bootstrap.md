# Go local runner toolchain bootstrap

- The Docker-local `generate-purchase-go` runner did not mirror the GitHub Actions `setup-go` step, so it relied on the container's apt-provided `golang-go` and failed when `_libraries/go_bindings/go.mod` requested `go 1.25`.
- Added local job bootstrap logic in `_local_build_pipeline/scripts/jobs/generate-purchase-go.sh` to read the required Go version from `go.mod`, install that toolchain into a temporary directory when the container version is too old, and prepend it to `PATH` before `go test`.
- This keeps the published GitHub workflow unchanged while bringing the Docker-local runner into parity with the language-version selection already used in CI.