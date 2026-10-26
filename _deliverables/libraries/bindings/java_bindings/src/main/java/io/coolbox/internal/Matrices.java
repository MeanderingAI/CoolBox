package io.coolbox.internal;

import com.sun.jna.Pointer;

/** Helpers for moving row-major matrices and native-owned buffers between Java and C. */
public final class Matrices {
    private Matrices() {
    }

    /** Returns the column count of a rectangular matrix, rejecting ragged or empty input. */
    public static int columns(double[][] matrix) {
        if (matrix.length == 0) {
            throw new IllegalArgumentException("matrix must have at least one row");
        }
        int cols = matrix[0].length;
        for (double[] row : matrix) {
            if (row.length != cols) {
                throw new IllegalArgumentException("matrix rows must all have the same length");
            }
        }
        return cols;
    }

    public static double[] flatten(double[][] matrix) {
        if (matrix.length == 0) {
            return new double[0];
        }
        int cols = columns(matrix);
        double[] out = new double[matrix.length * cols];
        for (int r = 0; r < matrix.length; r++) {
            System.arraycopy(matrix[r], 0, out, r * cols, cols);
        }
        return out;
    }

    public static double[][] unflatten(double[] values, int rows, int cols) {
        if ((long) rows * cols != values.length) {
            throw new IllegalArgumentException("buffer length does not match rows * cols");
        }
        double[][] out = new double[rows][cols];
        for (int r = 0; r < rows; r++) {
            System.arraycopy(values, r * cols, out[r], 0, cols);
        }
        return out;
    }

    /** Copies a native {@code double*} of {@code count} elements, then frees it with {@code coolbox_java_free}. */
    public static double[] takeDoubles(Pointer pointer, long count) {
        if (pointer == null) {
            return new double[0];
        }
        try {
            return pointer.getDoubleArray(0, Math.toIntExact(count));
        } finally {
            BridgeCore.INSTANCE.coolbox_java_free(pointer);
        }
    }

    /** Copies a native row-major {@code double*} matrix, then frees it with {@code coolbox_java_free}. */
    public static double[][] takeMatrix(Pointer pointer, long rows, long cols) {
        return unflatten(takeDoubles(pointer, rows * cols), Math.toIntExact(rows), Math.toIntExact(cols));
    }

    /** Copies a native {@code int64_t*}, then frees it with {@code coolbox_java_free}. */
    public static long[] takeLongs(Pointer pointer, long count) {
        if (pointer == null) {
            return new long[0];
        }
        try {
            return pointer.getLongArray(0, Math.toIntExact(count));
        } finally {
            BridgeCore.INSTANCE.coolbox_java_free(pointer);
        }
    }

    /** Copies a native {@code int32_t*}, then frees it with {@code coolbox_java_free}. */
    public static int[] takeInts(Pointer pointer, long count) {
        if (pointer == null) {
            return new int[0];
        }
        try {
            return pointer.getIntArray(0, Math.toIntExact(count));
        } finally {
            BridgeCore.INSTANCE.coolbox_java_free(pointer);
        }
    }

    /** Copies a malloc'ed UTF-8 C string, then frees it with {@code coolbox_java_free}. */
    public static String takeString(Pointer pointer) {
        if (pointer == null) {
            return null;
        }
        try {
            return pointer.getString(0, "UTF-8");
        } finally {
            BridgeCore.INSTANCE.coolbox_java_free(pointer);
        }
    }
}
