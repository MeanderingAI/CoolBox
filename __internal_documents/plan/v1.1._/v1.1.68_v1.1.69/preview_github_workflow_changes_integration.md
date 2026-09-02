
# Integration: GitHub Workflow Change Preview

## Features Added
- **Makefile**: Added a `preview-github-workflow-changes` target to run the Python script and preview all GitHub Actions workflow changes as a unified diff.
- **tasks.json**: Added a VS Code task "Preview GitHub Workflow Changes" to run the Makefile target from the VS Code Task Runner.
- **Script**: The script `preview_github_workflow_changes.py` shows a diff between original and current workflow YAML files.

## Usage
- **Command line**: Run `make preview-github-workflow-changes` to see a unified diff of workflow changes.
- **VS Code**: Run the "Preview GitHub Workflow Changes" task from the Task Runner (Terminal > Run Task...).

## Rationale
This integration makes it easy to review all workflow YAML changes before committing, improving CI/CD reliability and auditability.

---
**Date:** 2026-04-19
**Author:** GitHub Copilot
