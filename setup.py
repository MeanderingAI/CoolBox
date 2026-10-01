from pathlib import Path
import os
import runpy


project_root = Path(__file__).resolve().parent
bindings_root = project_root / "_deliverables" / "libraries" / "bindings" / "python_bindings"

os.chdir(bindings_root)
runpy.run_path(str(bindings_root / "setup.py"), run_name="__main__")