from pathlib import Path
import os
import runpy


project_root = Path(__file__).resolve().parent


def resolve_bindings_root(root: Path) -> Path:
	env_override = os.environ.get("COOLBOX_PYTHON_BINDINGS_DIR", "").strip()
	if env_override:
		candidate = Path(env_override)
		if not candidate.is_absolute():
			candidate = root / candidate
		if (candidate / "setup.py").exists():
			return candidate

	candidates = [
		root / "_deliverables" / "libraries" / "bindings" / "python_bindings",
		root / "_libraries" / "python_bindings",
	]

	for candidate in candidates:
		if (candidate / "setup.py").exists():
			return candidate

	checked = "\n  - ".join(str(path) for path in candidates)
	raise FileNotFoundError(
		"Unable to locate python bindings setup.py. Checked:\n"
		f"  - {checked}"
	)


bindings_root = resolve_bindings_root(project_root)

os.chdir(bindings_root)
runpy.run_path(str(bindings_root / "setup.py"), run_name="__main__")