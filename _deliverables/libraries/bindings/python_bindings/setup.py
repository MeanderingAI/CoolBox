from typing import Union
import os
from pathlib import Path
import glob
import shutil
import sys

import pybind11
from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup
from setuptools.command.build_ext import build_ext as build_ext_orig
class build_ext_with_move(build_ext_orig):
    def run(self):
        super().run()
        # Move built ml_core*.so into ml_toolbox/ for in-place builds
        search_dirs = ['.', self.build_lib if hasattr(self, 'build_lib') else None]
        found = False
        for search_dir in filter(None, search_dirs):
            for so_file in glob.glob(os.path.join(search_dir, "ml_core*.so")):
                dest = os.path.join("ml_toolbox", os.path.basename(so_file))
                print(f"[post-build] Moving {so_file} -> {dest}")
                shutil.move(so_file, dest)
                found = True
        if not found:
            print("[post-build] No ml_core*.so file found to move.")


project_root = Path(__file__).resolve().parent
repo_root = project_root.parent.parent.parent.parent  # _deliverables/libraries/bindings/python_bindings -> repo root
include_root = project_root / "include"
src_root = project_root / "src"
vendor_include_root = project_root / "vendor_include"
vendor_src_root = project_root / "vendor_src"
vendor_graphics_header = "vendor_include/GRAPHICS/charts/headers/graphics.h"
vendor_wave_header = "vendor_include/MISC/wave_generator/headers/wave_generator.hpp"
vendor_matrix_headers_dir = project_root / "vendor_include" / "MATRIX" / "headers"
timer_headers_dir = repo_root / "_deliverables/libraries/groups/Generics/timer/headers"
vendor_graphics_source = "vendor_src/GRAPHICS/charts/source/graphics.cpp"
vendor_wave_source = "vendor_src/MISC/wave_generator/source/wave_generator.cpp"

repo_graphics_header_candidates = [
    repo_root / "_deliverables/libraries/groups/app_builder/GRAPHICS/charts/headers/graphics.h",
    repo_root / "_libraries/packages/GRAPHICS/charts/headers/graphics.h",
]
repo_wave_header_candidates = [
    repo_root / "_deliverables/libraries/groups/trekker/MISC/wave_generator/headers/wave_generator.hpp",
    repo_root / "_libraries/packages/MISC/wave_generator/headers/wave_generator.hpp",
]
repo_graphics_source_candidates = [
    repo_root / "_deliverables/libraries/groups/app_builder/GRAPHICS/charts/source/graphics.cpp",
    repo_root / "_libraries/packages/GRAPHICS/charts/source/graphics.cpp",
]
repo_wave_source_candidates = [
    repo_root / "_deliverables/libraries/groups/trekker/MISC/wave_generator/source/wave_generator.cpp",
    repo_root / "_libraries/packages/MISC/wave_generator/source/wave_generator.cpp",
]
repo_matrix_header_candidates = [
    repo_root / "_deliverables/libraries/groups/cool_car/MATRIX/headers",
    project_root.parent.parent / "groups/cool_car/MATRIX/headers",
]
repo_cool_car_root_candidates = [
    repo_root / "_deliverables/libraries/groups/cool_car",
    project_root.parent.parent / "groups/cool_car",
]

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


def sync_vendor_file(repo_source: Union[Path, str], vendored_path: Union[Path, str]) -> Path:
    if repo_source.exists():
        vendored_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(repo_source, vendored_path)

    if not vendored_path.exists():
        raise FileNotFoundError(f"Required vendored file not found: {vendored_path}")

    return vendored_path


def sync_vendor_directory(repo_source_dir: Union[Path, str], vendored_dir: Union[Path, str], patterns) -> Path:
    repo_source_dir = Path(repo_source_dir)
    vendored_dir = Path(vendored_dir)

    if repo_source_dir.exists():
        vendored_dir.mkdir(parents=True, exist_ok=True)
        for pattern in patterns:
            for source in repo_source_dir.glob(pattern):
                if source.is_file():
                    shutil.copy2(source, vendored_dir / source.name)

    if not vendored_dir.exists():
        raise FileNotFoundError(f"Required vendored directory not found: {vendored_dir}")

    return vendored_dir


def resolve_repo_source(candidates) -> Path:
    for candidate in candidates:
        if candidate.exists():
            return candidate
    # Return first candidate to preserve deterministic error messages if none exist.
    return candidates[0]


def to_setup_relative_path(path: Union[Path, str]) -> str:
    path_obj = Path(path)

    if not path_obj.is_absolute():
        return path_obj.as_posix()

    normalized_path = Path(os.path.normpath(str(path_obj)))
    normalized_root = Path(os.path.normpath(str(project_root)))

    try:
        return normalized_path.relative_to(normalized_root).as_posix()
    except ValueError:
        return Path(os.path.relpath(normalized_path, normalized_root)).as_posix()


def candidate_library_filenames(name: str):
    if sys.platform.startswith("win"):
        return [f"{name}.lib"]
    if sys.platform == "darwin":
        return [f"lib{name}.dylib", f"lib{name}.a"]
    return [f"lib{name}.so", f"lib{name}.a"]


def resolve_link_inputs(search_root: Path, library_names):
    library_dirs = []
    extra_objects = []
    unresolved = []
    seen_dirs = set()

    for library_name in library_names:
        resolved_path = None
        for filename in candidate_library_filenames(library_name):
            matches = sorted(search_root.rglob(filename))
            if matches:
                resolved_path = matches[0]
                break

        if resolved_path is None:
            unresolved.append(library_name)
            continue

        extra_objects.append(str(resolved_path))
        parent = str(resolved_path.parent)
        if parent not in seen_dirs:
            seen_dirs.add(parent)
            library_dirs.append(parent)

    return library_dirs, extra_objects, unresolved


def env_flag(name: str) -> bool:
    value = os.environ.get(name, "")
    return value.strip().lower() in {"1", "true", "yes", "on"}


# On Windows CI we prefer to use the cleaned include files under
# `include/` to avoid parsing issues with any vendored files. Do not
# overwrite those files on Windows.
if sys.platform.startswith("win"):
    graphics_header = project_root / "vendor_include/GRAPHICS/charts/headers/graphics.h"
    # If a cleaned copy exists under our shipped include/ path, prefer it.
    cleaned = project_root / "include/GRAPHICS/charts/headers/graphics.h"
    if cleaned.exists():
        graphics_header = cleaned
    else:
        # Fallback: copy from repository packages if available.
        graphics_header = sync_vendor_file(
            resolve_repo_source(repo_graphics_header_candidates),
            project_root / vendor_graphics_header,
        )

    # Wave generator header: prefer cleaned include if present.
    wave_candidate = project_root / "include/MISC/wave_generator/headers/wave_generator.hpp"
    if wave_candidate.exists():
        wave_generator_header = wave_candidate
    else:
        wave_generator_header = sync_vendor_file(
            resolve_repo_source(repo_wave_header_candidates),
            project_root / vendor_wave_header,
        )

    # We do not need to sync the large vendor sources on Windows; they
    # are intentionally not compiled into the extension.
    graphics_source = project_root / vendor_graphics_source
    wave_generator_source = project_root / vendor_wave_source
else:
    graphics_header = sync_vendor_file(
        resolve_repo_source(repo_graphics_header_candidates),
        project_root / vendor_graphics_header,
    )
    wave_generator_header = sync_vendor_file(
        resolve_repo_source(repo_wave_header_candidates),
        project_root / vendor_wave_header,
    )
    graphics_source = sync_vendor_file(
        resolve_repo_source(repo_graphics_source_candidates),
        project_root / vendor_graphics_source,
    )
    wave_generator_source = sync_vendor_file(
        resolve_repo_source(repo_wave_source_candidates),
        project_root / vendor_wave_source,
    )


matrix_header_source_dir = resolve_repo_source(repo_matrix_header_candidates)
matrix_header_dir = sync_vendor_directory(
    matrix_header_source_dir,
    vendor_matrix_headers_dir,
    ["*.h", "*.hpp"],
)


eigen_candidates = [
    os.environ.get("EIGEN3_INCLUDE_DIR"),
    os.environ.get("EIGEN_INCLUDE_DIR"),
    # Repo-vendored Eigen (highest priority — always present)
    # project_root is 4 levels deep: _deliverables/libraries/bindings/python_bindings
    str(project_root.parent.parent.parent.parent / "external" / "eigen" / "eigen-3.4.0"),
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
    str(vendor_include_root),   # for "MISC/..." and "GRAPHICS/..." relative includes
    str(timer_headers_dir),
    str(matrix_header_dir),
    *(str(include_root / module_dir) for module_dir in module_dirs),
    str(graphics_header.parent),
    str(wave_generator_header.parent),
    # cool_car group headers: mytrix_eigen_compat.hpp, DL/layers, DL/loss, DL/optimizer, DL/wrapper
    # Include both the root (for "DL/..." paths) and MATRIX/headers (for bare includes)
    *existing_dirs(repo_cool_car_root_candidates),
    *existing_dirs(repo_matrix_header_candidates),
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

source_files = [
    "py_ml_core.cpp",
    "src/pde_spde_bindings.cpp",
    "src/timer_bindings.cpp",
    "src/synthetic_data_bindings.cpp",
]
for module_dir in source_modules:
    module_sources = sorted(glob.glob(f"src/{module_dir}/*.cpp"))
    if module_dir == "deep_learning":
        module_sources = [path for path in module_sources if not path.endswith("templates.cpp")]
    source_files.extend(module_sources)

# Do not compile large C++ vendor sources into the Python extension; instead
# link against the project's shared libraries. Users should build the C++ libs
# (via CMake) and point `COOLBOX_LIB_DIR` to the directory containing them.

# source_files.extend([
#     vendor_graphics_source,
#     vendor_wave_source,
# ])

requested_libraries = [
    lib for lib in os.environ.get("COOLBOX_LIBS", "charts,wave_generator_utils").split(",") if lib
]
force_vendor_sources = env_flag("COOLBOX_PYTHON_FORCE_VENDOR_SOURCES") and not sys.platform.startswith("win")
link_search_root = Path(os.environ.get("COOLBOX_LIB_DIR", str(project_root.parent / "build")))
resolved_library_dirs = []
resolved_extra_objects = []
unresolved_libraries = list(requested_libraries)

if not force_vendor_sources:
    resolved_library_dirs, resolved_extra_objects, unresolved_libraries = resolve_link_inputs(
        link_search_root,
        requested_libraries,
    )

fallback_sources = {
    "charts": graphics_source,
    "wave_generator_utils": wave_generator_source,
}
remaining_unresolved = []
for library_name in unresolved_libraries:
    fallback_source = fallback_sources.get(library_name)
    if fallback_source is None or not Path(fallback_source).exists():
        remaining_unresolved.append(library_name)
        continue

    fallback_source_str = to_setup_relative_path(fallback_source)
    if fallback_source_str not in source_files:
        source_files.append(fallback_source_str)

unresolved_libraries = remaining_unresolved

source_files = [to_setup_relative_path(path) for path in source_files]

if requested_libraries:
    print(f"Requested native libraries: {requested_libraries}")
print(f"Native library search root: {link_search_root}")
print(f"Force vendored sources: {force_vendor_sources}")
if resolved_extra_objects:
    print(f"Resolved native library objects: {resolved_extra_objects}")
if unresolved_libraries:
    print(f"Libraries still requiring linker resolution: {unresolved_libraries}")
else:
    print("No unresolved native libraries remain after fallback resolution")

library_dirs = existing_dirs([
    str(link_search_root),
    *resolved_library_dirs,
])

runtime_library_dirs = []
if not sys.platform.startswith("win"):
    runtime_library_dirs = resolved_library_dirs[:]

ext_modules = [
    Pybind11Extension(
        "ml_toolbox.ml_core",
        source_files,
        include_dirs=include_dirs,
        cxx_std=17,
        extra_compile_args=extra_compile_args,
        # Link against built CoolBox C++ libraries. Set COOLBOX_LIB_DIR to
        # the build output directory containing libcharts(.a/.so/.dylib)
        # and libwave_generator_utils.
        library_dirs=library_dirs,
        libraries=unresolved_libraries,
        extra_objects=resolved_extra_objects,
        runtime_library_dirs=runtime_library_dirs,
    ),
]

setup(
    packages=["ml_toolbox"],
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext_with_move},
    zip_safe=False,
)