from pybind11.setup_helpers import Pybind11Extension, build_ext
from pybind11 import get_cmake_dir
import pybind11
from setuptools import setup, Extension
import os
import glob

# Get the absolute path to the project root
project_root = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(project_root)

# Include directories
include_dirs = [
    # pybind11 includes
    pybind11.get_include(),
    # Project includes
    os.path.join(parent_dir, "include"),
    os.path.join(parent_dir, "include", "bayesian_network"),
    os.path.join(parent_dir, "include", "decision_tree"),
    os.path.join(parent_dir, "include", "dimensionality_reduction"),
    os.path.join(parent_dir, "include", "generalized_linear_model"),
    os.path.join(parent_dir, "include", "hidden_markov_model"),
    os.path.join(parent_dir, "include", "multi_arm_bandit"),
    os.path.join(parent_dir, "include", "support_vector_machine"),
    os.path.join(parent_dir, "include", "tracker"),
    # Eigen includes (assuming it's in the build directory)
    os.path.join(parent_dir, "build", "eigen-src"),
    # System includes for Eigen
    "/usr/include/eigen3",
    "/usr/local/include/eigen3",
]

# Source files - collect all .cpp files from src directories
source_files = ["py_ml_core.cpp"]

# Add all implementation files
src_dirs = [
    "src/decision_tree",
    "src/support_vector_machine", 
    "src/bayesian_network",
    "src/hidden_markov_model",
    "src/generalized_linear_model",
    "src/multi_arm_bandit",
    "src/tracker",
    "src/dimensionality_reduction",
    "src/deep_learning",
    "src/distributed",
    "src/computer_vision",
    "src/time_series",
    "src/nlp",
    "src/json",
    "src/sql",
    "src/rest_api"
]

for src_dir in src_dirs:
    full_path = os.path.join(parent_dir, src_dir)
    if os.path.exists(full_path):
        cpp_files = glob.glob(os.path.join(full_path, "*.cpp"))
        # Make paths relative to python_bindings directory
        relative_files = [os.path.relpath(f, project_root) for f in cpp_files]
        source_files.extend(relative_files)

# Compiler flags
compile_args = [
    "-std=c++17",
    "-O3",
    "-Wall",
    "-shared",
    "-fPIC",
]

# Define the extension
ext_modules = [
    Pybind11Extension(
        "ml_core",
        source_files,
        include_dirs=include_dirs,
        libraries=["sqlite3"],
        cxx_std=17,
        extra_compile_args=compile_args,
    ),
]

setup(
    name="ml-toolbox",
    version="0.2.0",
    author="ML Core Team",
    author_email="",
    description="Comprehensive C++ Machine Learning Library with Python Bindings",
    long_description=open(os.path.join(project_root, "README.md")).read() if os.path.exists(os.path.join(project_root, "README.md")) else "A comprehensive machine learning library with Python bindings for decision trees, SVM, Bayesian networks, HMM, deep learning, distributed training, and more.",
    long_description_content_type="text/markdown",
    url="https://github.com/yourusername/ToolBox",
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
    zip_safe=False,
    python_requires=">=3.6",
    install_requires=[
        "numpy",
        "pybind11>=2.6.0",
    ],
    classifiers=[
        "Development Status :: 3 - Alpha",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: C++",
        "Topic :: Scientific/Engineering :: Artificial Intelligence",
        "Topic :: Software Development :: Libraries :: Python Modules",
    ],
)