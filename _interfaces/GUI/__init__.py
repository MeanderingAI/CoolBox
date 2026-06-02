
import os

def _repo_root():
    # This file lives at _interfaces/GUI/__init__.py
    # Two levels up: GUI/ → _interfaces/ → CoolBox/ (repo root)
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

def mount_gif_third_party(app, NoCacheStaticFiles):
    """Mount the /third_party/js_libs/gif static directory if it exists."""
    if os.path.isdir(GIF_THIRD_PARTY_DIR):
        app.mount(
            "/third_party/js_libs/gif",
            NoCacheStaticFiles(directory=GIF_THIRD_PARTY_DIR),
            name="gif-third-party"
        )

REPO_ROOT = _repo_root()
GIF_THIRD_PARTY_DIR = os.path.join(REPO_ROOT, "__init__", "third_party", "js_libs", "gif")
GITHUB_WORKFLOWS = os.path.join(REPO_ROOT, ".github", "workflows")
SCRIPT_PATH = os.path.join(REPO_ROOT, "_scripts", "preview_github_workflow_changes.py")
DSN_PATH = os.path.join(REPO_ROOT, "_local_build_pipeline", "tmp", "postgres_dsn.txt")