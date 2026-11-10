# CoolBox LLM Benchmark

This directory contains a reproducible MLPerf-style training benchmark for
`DecoderLanguageModel`. It is not an official MLPerf benchmark and its output
is explicitly marked `mlperf_compliant: false`.

Enable and build it with:

```bash
cmake -S . -B build -DCOOLBOX_BUILD_LLM_BENCHMARKS=ON
cmake --build build --target coolbox_llm_benchmark
```

Example Vulkan run:

```bash
COOLBOX_VULKAN_DEVICE=intel \
  ./build/DL_llm_build/coolbox_llm_benchmark \
  --workload small \
  --backend vulkan \
  --precision bf16 \
  --batch-size 8 \
  --warmup-steps 5 \
  --steps 50 \
  --json-output small-bf16.json \
  --csv-output small-bf16.csv
```

Available workloads are `tiny`, `small`, and `base`. The benchmark reports
training throughput, step-time percentiles, loss, parameter count, device and
precision metadata, and optional time-to-target-loss measurements. Input token
sequences are deterministic and synthetic so runs do not depend on external
datasets.

Set `COOLBOX_PROFILE_TRAINING=1` to emit decoder phase timings for
forward/loss, backward command recording, backward synchronization and gradient
norm reduction, and optimizer completion.
