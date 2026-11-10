package io.coolbox.opt;

import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.NativeLibraries;

interface OptNative extends Library {
    OptNative INSTANCE = NativeLibraries.bridge(OptNative.class);
    interface Objective extends Callback { double invoke(Pointer userData, Pointer values, long count); }
    interface Payoff extends Callback { int invoke(Pointer userData, Pointer path, long count, DoubleByReference payoff); }
    interface NowaLoss extends Callback { int invoke(Pointer userData, Pointer values, Pointer layerSizes, long layerCount, DoubleByReference loss); }
    Pointer coolbox_optimizer_create(int kind, double[] config, long count, Pointer error, long capacity);
    void coolbox_optimizer_free(Pointer handle);
    int coolbox_optimizer_run(Pointer handle, Objective objective, Pointer userData, double[] initial, long count, Pointer error, long capacity);
    long coolbox_optimizer_solution_size(Pointer handle);
    int coolbox_optimizer_copy_solution(Pointer handle, double[] output, long count, Pointer error, long capacity);
    double coolbox_optimizer_best_score(Pointer handle);
    double coolbox_optimizer_best_probability(Pointer handle);
    int coolbox_optimizer_quantum_bounds(Pointer handle, double[] output, long count, Pointer error, long capacity);
    int coolbox_optimizer_warp_point(Pointer handle, double[] input, long count, double[] output, int unwarp, Pointer error, long capacity);
    int coolbox_owa_naive_average(double[] local, long models, long parameters, double[] output, Pointer error, long capacity);
    int coolbox_owa_linear_regression(double[] local,long models,long parameters,double[] features,long rows,long featureCount,double[] targets,long targetCount,int iterations,double rate,double regularization,int simplex,double[] merged,double[] weights,DoubleByReference loss,Pointer error,long capacity);
    int coolbox_owa_cross_validation_linear_regression(double[] local,long models,long parameters,double[] features,long[] rowOffsets,double[] targets,long[] targetOffsets,long folds,long featureCount,int iterations,double rate,double regularization,int simplex,double[] losses,DoubleByReference mean,Pointer error,long capacity);
    int coolbox_owa_nonlinear_layers(double[] local,long models,long layerCount,long[] shapes,long[] shapeOffsets,long[] sizes,long[] valueOffsets,long[] dimensions,int iterations,double rate,double regularization,int simplex,NowaLoss loss,Pointer userData,double[] merged,double[] weights,DoubleByReference validation,Pointer error,long capacity);
    int coolbox_owa_weighted_tensor_average(double[] local,long models,long[] shape,long dimensions,double[] weights,long tensorSize,double[] merged,Pointer error,long capacity);
}

