#!/usr/bin/env python3
"""Compare CoolBox CPU dense matrix multiplication with NumPy's float64 matmul."""

import argparse
import json
import os
import platform
import statistics
import subprocess
import sys
import time

import numpy as np

from ml_toolbox import matrix


def parse_thread_count(value):
    if value.lower() == "auto":
        return 0
    try:
        count = int(value)
    except ValueError as error:
        raise argparse.ArgumentTypeError("threads must be 'auto' or a positive integer") from error
    if count < 1:
        raise argparse.ArgumentTypeError("threads must be 'auto' or a positive integer")
    return count


def median_seconds(operation, warmups, repetitions):
    for _ in range(warmups):
        operation()
    samples = []
    for _ in range(repetitions):
        start = time.perf_counter()
        operation()
        samples.append(time.perf_counter() - start)
    return statistics.median(samples)


def numpy_threadpools():
    try:
        from threadpoolctl import threadpool_info
    except ImportError:
        return None
    return threadpool_info()


def cpu_model_name():
    try:
        output = subprocess.run(
            ["lscpu", "-J"], capture_output=True, check=True, text=True
        )
        fields = {
            field["field"].rstrip(":").strip(): field["data"].strip()
            for field in json.loads(output.stdout)["lscpu"]
        }
        model = fields.get("Model name") or fields.get("Hardware")
        if model:
            return model
    except (FileNotFoundError, KeyError, json.JSONDecodeError, subprocess.CalledProcessError):
        pass

    try:
        with open("/proc/cpuinfo", encoding="utf-8") as cpuinfo:
            for line in cpuinfo:
                key, separator, value = line.partition(":")
                if separator and key.strip().lower() in {"model name", "hardware"}:
                    if value.strip():
                        return value.strip()
    except OSError:
        pass
    return platform.processor() or None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--sizes", type=int, nargs="+", default=[32, 64, 128, 256, 512, 1024]
    )
    parser.add_argument(
        "--threads", type=parse_thread_count, nargs="+", default=[0, 1, 2, 4]
    )
    parser.add_argument(
        "--backends", choices=("cpu", "eigen", "openblas"), nargs="+", default=["cpu"]
    )
    parser.add_argument("--output", help="also save the JSON report to this file")
    parser.add_argument("--warmups", type=int, default=2)
    parser.add_argument("--repetitions", type=int, default=7)
    parser.add_argument("--seed", type=int, default=20261019)
    args = parser.parse_args()
    if any(size < 1 for size in args.sizes):
        parser.error("matrix sizes must be positive")
    if args.warmups < 0 or args.repetitions < 1:
        parser.error("warmups must be non-negative and repetitions must be positive")
    if len(set(args.threads)) != len(args.threads):
        parser.error("thread settings must be unique")

    original_backend = matrix.get_backend()
    original_thread_limit = matrix.cpu_threads()
    backends = {
        "cpu": matrix.MatrixBackend.CPU,
        "eigen": matrix.MatrixBackend.EIGEN,
        "openblas": matrix.MatrixBackend.OPENBLAS,
    }
    rng = np.random.default_rng(args.seed)
    results = []
    try:
        for backend_name in args.backends:
            matrix.set_backend(backends[backend_name])
            for size in args.sizes:
                left_array = rng.standard_normal((size, size), dtype=np.float64)
                right_array = rng.standard_normal((size, size), dtype=np.float64)
                reference = left_array @ right_array

                # Native object and Python-list conversions happen outside timed regions.
                left_native = matrix.DenseMatrix(size, size, left_array.ravel().tolist())
                right_native = matrix.DenseMatrix(size, size, right_array.ravel().tolist())

                for configured_threads in args.threads:
                    matrix.set_cpu_threads(configured_threads)
                    native_result = left_native.multiply(right_native)
                    actual = np.asarray(native_result.data, dtype=np.float64).reshape(size, size)
                    if not np.allclose(actual, reference, rtol=1e-10, atol=1e-10):
                        raise AssertionError(
                            f"CoolBox result did not match NumPy for backend={backend_name}, "
                            f"size={size}, threads={configured_threads or 'auto'}"
                        )

                    native_seconds = median_seconds(
                        lambda: left_native.multiply(right_native),
                        args.warmups,
                        args.repetitions,
                    )
                    numpy_seconds = median_seconds(
                        lambda: left_array @ right_array,
                        args.warmups,
                        args.repetitions,
                    )
                    operation_count = 2.0 * size**3
                    results.append(
                        {
                            "backend": backend_name,
                            "size": size,
                            "configured_cpu_threads": configured_threads,
                            "configured_cpu_threads_label": (
                                "auto" if configured_threads == 0 else str(configured_threads)
                            ),
                            "resolved_cpu_thread_budget": matrix.cpu_threads(),
                            "coolbox_median_seconds": native_seconds,
                            "numpy_median_seconds": numpy_seconds,
                            "coolbox_over_numpy_speedup": numpy_seconds / native_seconds,
                            "coolbox_gflops": operation_count / native_seconds / 1e9,
                            "numpy_gflops": operation_count / numpy_seconds / 1e9,
                            "max_absolute_error": float(np.max(np.abs(actual - reference))),
                        }
                    )
    finally:
        matrix.set_cpu_threads(original_thread_limit)
        matrix.set_backend(original_backend)

    output = {
        "metadata": {
            "python": sys.version.split()[0],
            "numpy": np.__version__,
            "platform": platform.platform(),
            "machine": platform.machine(),
            "processor": cpu_model_name(),
            "logical_cpu_count": os.cpu_count(),
            "seed": args.seed,
            "warmups": args.warmups,
            "repetitions": args.repetitions,
            "backends": args.backends,
            "numpy_threadpools": numpy_threadpools(),
            "note": (
                "Configured and resolved CoolBox thread budgets are reported; "
                "the resolved value is a budget, not a measured per-operation "
                "worker count. NumPy thread pools are "
                "reported when threadpoolctl is installed and otherwise follow the "
                "environment's BLAS configuration."
            ),
        },
        "results": results,
    }
    rendered = json.dumps(output, indent=2)
    if args.output:
        with open(args.output, "w", encoding="utf-8") as report:
            report.write(rendered)
            report.write("\n")
    print(rendered)


if __name__ == "__main__":
    main()
