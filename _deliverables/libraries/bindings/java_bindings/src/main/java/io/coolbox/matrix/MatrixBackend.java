package io.coolbox.matrix;

/** Matrix backend selection (Python: {@code matrix.MatrixBackend}). */
public enum MatrixBackend { CPU, EIGEN, AUTO, OPEN_CL, CUDA, METAL, OPEN_BLAS }
