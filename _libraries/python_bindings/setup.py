from pathlib import Path
import glob

import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup


project_root = Path(__file__).resolve().parent
include_root = project_root / "include"
src_root = project_root / "src"

module_dirs = [
    "decision_tree",
    "support_vector_machine",
    "bayesian_network",
    "hidden_markov_model",
    "generalized_linear_model",
    "multi_arm_bandit",
    "tracker",
    "dimensionality_reduction",
    "deep_learning",
    "computer_vision",
    "time_series",
    "nlp",
    "distributed",
    "rest_api",
]

source_modules = [module_dir for module_dir in module_dirs if module_dir != "rest_api"]

include_dirs = [
    pybind11.get_include(),
    str(include_root),
    *(str(include_root / module_dir) for module_dir in module_dirs),
    str(project_root.parent.parent / "eigen-src"),
    str(project_root.parent.parent / "build" / "eigen-src"),
    "/usr/include/eigen3",
    "/usr/local/include/eigen3",
    "/opt/homebrew/include/eigen3",
]

source_files = [project_root / "py_ml_core.cpp"]
for module_dir in source_modules:
    module_sources = sorted(glob.glob(str(src_root / module_dir / "*.cpp")))
    if module_dir == "deep_learning":
        module_sources = [path for path in module_sources if not path.endswith("templates.cpp")]
    source_files.extend(module_sources)

source_files = [str(Path(path).relative_to(project_root)) for path in source_files]

ext_modules = [
    Pybind11Extension(
        "ml_core",
        source_files,
        include_dirs=include_dirs,
        cxx_std=17,
        extra_compile_args=["-O3", "-Wall"],
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)