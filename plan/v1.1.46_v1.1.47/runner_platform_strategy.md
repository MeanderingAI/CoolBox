# Runner Platform Strategy

## Summary
- Linux can continue using Docker-backed local and CI runners.
- Windows can use Docker-backed runners only on Windows hosts, and only for Windows container workloads.
- macOS does not have a Docker runner model equivalent to Linux containers; macOS validation should use native or VM-based macOS runners instead.

## Linux
- Linux is the best fit for the current Docker-backed validation flow.
- Container images are first-class, reproducible, and easy to run locally and in CI.
- This should remain the default path for containerized validation jobs.

## Windows
- Windows containers are possible, but they are not equivalent to Linux containers.
- Practical constraints:
  - They require Windows hosts.
  - The container base image and host kernel version must stay compatible.
  - They are less portable and generally heavier than Linux containers.
  - GUI-oriented or desktop-integration validation is usually better on a native Windows runner than inside a Windows container.
- Recommendation:
  - Keep Docker-based Windows validation only for narrow CLI or service scenarios that truly benefit from containerization.
  - Use native Windows runners for broader build, integration, and UI-adjacent validation.

## macOS
- There is no Docker-for-macOS container model equivalent to Linux where macOS workloads run as normal Docker containers.
- Practical constraints:
  - macOS workloads require Apple hardware for compliant virtualization.
  - Docker Desktop on macOS runs Linux containers in a VM; it does not provide native macOS containers.
  - GitHub-hosted and self-hosted macOS jobs must run on macOS runners directly or via macOS VM tooling layered on Apple hardware.
- Recommendation:
  - Use native macOS runners for build and test validation.
  - If reproducibility or elasticity is needed, use VM-based macOS runner orchestration such as Anka, Orka, Tart, or Apple Virtualization.framework-based provisioning rather than Docker.

## Proposed Direction
- Keep `_local_build_pipeline/docker/linux-ci.Dockerfile` and the current Linux container flow as the portable container baseline.
- Add native Windows and macOS validation scripts that mirror the Linux job inputs and outputs where practical.
- Treat Linux Docker, Windows native, and macOS native/VM as three separate execution backends sharing the same logical validation contract rather than forcing all three into the same container runtime model.

## Implementation Follow-Up
- The repository-specific implementation breakdown for this strategy is tracked in `runner_platform_implementation.md` in the same release-plan folder.