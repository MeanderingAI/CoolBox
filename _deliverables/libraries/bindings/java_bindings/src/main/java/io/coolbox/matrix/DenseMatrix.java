package io.coolbox.matrix;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Dense row-major numeric matrix (Python: {@code matrix.DenseMatrix}). */
public final class DenseMatrix extends NativeHandle {
    public DenseMatrix(int rows, int columns) { this(rows, columns, new double[checkedProduct(rows, columns)]); }
    public DenseMatrix(double[][] values) { this(values.length, Matrices.columns(values), Matrices.flatten(values)); }
    public DenseMatrix(int rows, int columns, double[] values) { super(create(rows, columns, values), MatrixNative.INSTANCE::coolbox_matrix_free); }
    private static int checkedProduct(int rows, int columns) { if (rows <= 0 || columns <= 0) throw new IllegalArgumentException("matrix dimensions must be positive"); return Math.multiplyExact(rows, columns); }
    private static Pointer create(int rows, int columns, double[] values) { int count = checkedProduct(rows, columns); if (values.length != count) throw new IllegalArgumentException("value count must equal rows * columns"); ErrorBuffer e = new ErrorBuffer(); return e.check(MatrixNative.INSTANCE.coolbox_matrix_new(rows, columns, values, count, e.pointer(), e.capacity())); }
    static DenseMatrix wrap(Pointer pointer) { return new DenseMatrix(pointer); }
    private DenseMatrix(Pointer pointer) { super(pointer, MatrixNative.INSTANCE::coolbox_matrix_free); }
    public static DenseMatrix identity(int size) { ErrorBuffer e = new ErrorBuffer(); return wrap(e.check(MatrixNative.INSTANCE.coolbox_matrix_identity(size, e.pointer(), e.capacity()))); }
    public int rows() { return Math.toIntExact(MatrixNative.INSTANCE.coolbox_matrix_rows(handle())); }
    public int columns() { return Math.toIntExact(MatrixNative.INSTANCE.coolbox_matrix_columns(handle())); }
    public double at(int row, int column) { DoubleByReference v = new DoubleByReference(); ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_matrix_get(handle(), row, column, v, e.pointer(), e.capacity())); return v.getValue(); }
    public void set(int row, int column, double value) { ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_matrix_set(handle(), row, column, value, e.pointer(), e.capacity())); }
    public DenseMatrix add(DenseMatrix other) { return binary(other, 0, CpuOptimization.AUTO); }
    public DenseMatrix add(DenseMatrix other, CpuOptimization opt) { return binary(other, 0, opt); }
    public DenseMatrix subtract(DenseMatrix other) { return binary(other, 1, CpuOptimization.AUTO); }
    public DenseMatrix multiply(DenseMatrix other) { return binary(other, 2, CpuOptimization.AUTO); }
    public DenseMatrix multiply(DenseMatrix other, CpuOptimization opt) { return binary(other, 2, opt); }
    private DenseMatrix binary(DenseMatrix other, int op, CpuOptimization opt) { ErrorBuffer e = new ErrorBuffer(); return wrap(e.check(MatrixNative.INSTANCE.coolbox_matrix_binary(handle(), other.handle(), op, opt.ordinal(), e.pointer(), e.capacity()))); }
    public DenseMatrix transpose() { ErrorBuffer e = new ErrorBuffer(); return wrap(e.check(MatrixNative.INSTANCE.coolbox_matrix_transpose(handle(), e.pointer(), e.capacity()))); }
    public DenseMatrix rowwiseAdd(DenseVector vector) { return rowwiseAdd(vector, CpuOptimization.AUTO); }
    public DenseMatrix rowwiseAdd(DenseVector vector, CpuOptimization opt) { ErrorBuffer e = new ErrorBuffer(); return wrap(e.check(MatrixNative.INSTANCE.coolbox_matrix_rowwise_add(handle(), vector.handle(), opt.ordinal(), e.pointer(), e.capacity()))); }
    public double norm() { return MatrixNative.INSTANCE.coolbox_matrix_norm(handle()); }
    public double[][] toArray() { int r = rows(), c = columns(); double[] data = new double[Math.multiplyExact(r, c)]; ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_matrix_copy_data(handle(), data, data.length, e.pointer(), e.capacity())); return Matrices.unflatten(data, r, c); }
}
