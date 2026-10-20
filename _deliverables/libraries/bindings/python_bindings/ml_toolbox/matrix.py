"""Dense matrix and vector API backed by the CoolBox MATRIX library."""

from .ml_core import matrix as _native

CpuOptimization = _native.CpuOptimization
MatrixBackend = _native.MatrixBackend
DenseMatrix = _native.DenseMatrix
DenseVector = _native.DenseVector
backend_available = _native.backend_available
backend_name = _native.backend_name
cpu_optimization_available = _native.cpu_optimization_available
cpu_optimization_name = _native.cpu_optimization_name
cpu_threads = _native.cpu_threads
get_backend = _native.get_backend
get_active_backend = _native.get_active_backend
set_backend = _native.set_backend
set_cpu_threads = _native.set_cpu_threads

__all__ = [
    "CpuOptimization",
    "MatrixBackend",
    "DenseMatrix",
    "DenseVector",
    "backend_available",
    "backend_name",
    "cpu_optimization_available",
    "cpu_optimization_name",
    "cpu_threads",
    "get_backend",
    "get_active_backend",
    "set_backend",
    "set_cpu_threads",
]
