package io.coolbox.matrix;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Dense numeric vector (Python: {@code matrix.DenseVector}). */
public final class DenseVector extends NativeHandle {
    public DenseVector(int size) { this(new double[checkedSize(size)]); }
    public DenseVector(double[] values) { super(create(values), MatrixNative.INSTANCE::coolbox_vector_free); }
    private static int checkedSize(int size) { if (size < 0) throw new IllegalArgumentException("size must be non-negative"); return size; }
    private static Pointer create(double[] values) { ErrorBuffer e = new ErrorBuffer(); return e.check(MatrixNative.INSTANCE.coolbox_vector_new(values, values.length, e.pointer(), e.capacity())); }
    public int size() { return Math.toIntExact(MatrixNative.INSTANCE.coolbox_vector_size(handle())); }
    public double at(int index) { DoubleByReference v = new DoubleByReference(); ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_vector_get(handle(), index, v, e.pointer(), e.capacity())); return v.getValue(); }
    public void set(int index, double value) { ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_vector_set(handle(), index, value, e.pointer(), e.capacity())); }
    public double norm() { return MatrixNative.INSTANCE.coolbox_vector_norm(handle()); }
    public double dot(DenseVector other) { return dot(other, CpuOptimization.AUTO); }
    public double dot(DenseVector other, CpuOptimization optimization) { DoubleByReference v = new DoubleByReference(); ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_vector_dot(handle(), other.handle(), optimization.ordinal(), v, e.pointer(), e.capacity())); return v.getValue(); }
    public DenseVector add(DenseVector other) { return add(other, CpuOptimization.AUTO); }
    public DenseVector add(DenseVector other, CpuOptimization optimization) { ErrorBuffer e = new ErrorBuffer(); return new DenseVector(e.check(MatrixNative.INSTANCE.coolbox_vector_add(handle(), other.handle(), optimization.ordinal(), e.pointer(), e.capacity()))); }
    private DenseVector(Pointer pointer) { super(pointer, MatrixNative.INSTANCE::coolbox_vector_free); }
    public DenseMatrix transpose() { ErrorBuffer e = new ErrorBuffer(); return DenseMatrix.wrap(e.check(MatrixNative.INSTANCE.coolbox_vector_transpose(handle(), e.pointer(), e.capacity()))); }
    public double[] toArray() { double[] out = new double[size()]; ErrorBuffer e = new ErrorBuffer(); e.check(MatrixNative.INSTANCE.coolbox_vector_copy_data(handle(), out, out.length, e.pointer(), e.capacity())); return out; }
}
