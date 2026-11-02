package io.coolbox.matrix;

/** CPU kernel selection (Python: {@code matrix.CpuOptimization}). */
public enum CpuOptimization { AUTO, SCALAR, AVX2_FMA, NEON }
