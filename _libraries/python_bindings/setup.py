import os
from pathlib import Path
import glob
import shutil
import sys

import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup


project_root = Path(__file__).resolve().parent
repo_root = project_root.parent.parent
include_root = project_root / "include"
src_root = project_root / "src"
vendor_include_root = project_root / "vendor_include"
vendor_src_root = project_root / "vendor_src"
vendor_graphics_header = "vendor_include/GRAPHICS/charts/headers/graphics.h"
vendor_wave_header = "vendor_include/MISC/wave_generator/headers/wave_generator.hpp"
vendor_graphics_source = "vendor_src/GRAPHICS/charts/source/graphics.cpp"
vendor_wave_source = "vendor_src/MISC/wave_generator/source/wave_generator.cpp"

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
    "graphics_misc",
    "rest_api",
]

source_modules = [module_dir for module_dir in module_dirs if module_dir != "rest_api"]


def existing_dirs(paths):
    seen = set()
    result = []
    for path in paths:
        if not path:
            continue
        normalized = str(Path(path))
        if normalized in seen:
            continue
        if Path(normalized).exists():
            seen.add(normalized)
            result.append(normalized)
    return result


def resolve_eigen_include_dirs(paths):
    seen = set()
    resolved = []
    for raw_path in paths:
        if not raw_path:
            continue

        candidate = Path(raw_path)
        variants = [candidate]

        if candidate.name.lower() != "eigen3":
            variants.append(candidate / "eigen3")

        for variant in variants:
            eigen_core = variant / "Eigen" / "Core"
            unsupported = variant / "unsupported"
            if not eigen_core.exists() and not unsupported.exists():
                continue

            normalized = str(variant)
            if normalized in seen:
                continue
            seen.add(normalized)
            resolved.append(normalized)

    return resolved


def sync_vendor_file(repo_source: Path, vendored_path: Path) -> Path:
    if repo_source.exists():
        vendored_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(repo_source, vendored_path)

    if not vendored_path.exists():
        raise FileNotFoundError(f"Required vendored file not found: {vendored_path}")

    return vendored_path


graphics_header = sync_vendor_file(
    repo_root / "_libraries/backages/GRAPHICS/charts/headers/graphics.h",
    project_root / vendor_graphics_header,
)
wave_generator_header = sync_vendor_file(
    repo_root / "_libraries/backages/MISC/wave_generator/headers/wave_generator.hpp",
    project_root / vendor_wave_header,
)
graphics_source = sync_vendor_file(
    repo_root / "_libraries/backages/GRAPHICS/charts/source/graphics.cpp",
    project_root / vendor_graphics_source,
)
wave_generator_source = sync_vendor_file(
    repo_root / "_libraries/backages/MISC/wave_generator/source/wave_generator.cpp",
    project_root / vendor_wave_source,
)


eigen_candidates = [
    os.environ.get("EIGEN3_INCLUDE_DIR"),
    os.environ.get("EIGEN_INCLUDE_DIR"),
    str(project_root.parent.parent / "eigen-src"),
    str(project_root.parent.parent / "eigen-src" / "eigen3"),
    str(project_root.parent.parent / "build" / "eigen-src"),
    str(project_root.parent.parent / "build" / "eigen-src" / "eigen3"),
    "/usr/include/eigen3",
    "/usr/local/include/eigen3",
    "/opt/homebrew/include/eigen3",
    r"C:\vcpkg\installed\x64-windows\include\eigen3",
    r"C:\vcpkg\installed\x64-windows\include",
    r"C:\msys64\mingw64\include\eigen3",
    r"C:\msys64\mingw64\include",
    r"C:\tools\msys64\mingw64\include\eigen3",
    r"C:\tools\msys64\mingw64\include",
]

include_dirs = [
    pybind11.get_include(),
    str(include_root),
    *(str(include_root / module_dir) for module_dir in module_dirs),
    str(graphics_header.parent),
    str(wave_generator_header.parent),
    *existing_dirs(
        [
            repo_root / "build/_deps/stb-src",
            repo_root / "build/container-check/_deps/stb-src",
            repo_root / "build/crypto-check/_deps/stb-src",
        ]
    ),
    *resolve_eigen_include_dirs(eigen_candidates),
]

extra_compile_args = ["/O2", "/EHsc"] if sys.platform.startswith("win") else ["-O3", "-Wall"]

source_files = ["py_ml_core.cpp"]
for module_dir in source_modules:
    module_sources = sorted(glob.glob(f"src/{module_dir}/*.cpp"))
    if module_dir == "deep_learning":
        module_sources = [path for path in module_sources if not path.endswith("templates.cpp")]
    source_files.extend(module_sources)

source_files.extend(
    [
        vendor_graphics_source,
        vendor_wave_source,
    ]
)

ext_modules = [
    Pybind11Extension(
        "ml_core",
        source_files,
        include_dirs=include_dirs,
        cxx_std=17,
        extra_compile_args=extra_compile_args,
    ),
]

setup(
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
)