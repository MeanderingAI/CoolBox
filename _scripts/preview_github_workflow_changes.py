"""
preview_github_workflow_changes.py

A script to preview all changes made to GitHub Actions workflow YAML files in the current workspace.
Shows a unified diff for each changed workflow file.
"""
import difflib
import os
from pathlib import Path

WORKFLOW_DIR = Path('.github/workflows')
PLAN_DIR = Path('plan/v1.1.67_v1.1.68')

# List of workflow files to check (add more as needed)
WORKFLOW_FILES = [
    'build-libs.yaml',
    'build-purchase-pipeline.yaml',
]

def read_file(path):
    try:
        with open(path, encoding='utf-8') as f:
            return f.readlines()
    except FileNotFoundError:
        return []

def show_diff(before, after, filename):
    diff = difflib.unified_diff(
        before, after,
        fromfile=f'a/{filename}',
        tofile=f'b/{filename}',
        lineterm=''  # Don't add extra newlines
    )
    return '\n'.join(diff)

def main():
    print("GitHub Actions Workflow Change Preview\n" + "="*40)
    for wf in WORKFLOW_FILES:
        orig_path = PLAN_DIR / f'{wf}.orig'
        new_path = WORKFLOW_DIR / wf
        before = read_file(orig_path)
        after = read_file(new_path)
        if before and after:
            print(f'\n--- {wf} ---')
            print(show_diff(before, after, wf))
        elif after:
            print(f'\n--- {wf} (no original, showing current) ---')
            print(''.join(after))
        else:
            print(f'\n--- {wf} (not found) ---')

if __name__ == '__main__':
    main()
