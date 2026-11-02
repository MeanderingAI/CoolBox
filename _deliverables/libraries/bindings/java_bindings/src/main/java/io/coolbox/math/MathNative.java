package io.coolbox.math;
import com.sun.jna.Library;import io.coolbox.internal.NativeLibraries;import io.coolbox.internal.ErrorBuffer;
interface MathNative extends Library {MathNative INSTANCE=NativeLibraries.bridge(MathNative.class);int coolbox_math_heat(int dimensions,int stochastic,double[] input,long nx,long ny,long nz,double alpha,double sigma,double dx,double dy,double dz,double dt,int steps,int seed,double[] output,com.sun.jna.Pointer error,long capacity);}
