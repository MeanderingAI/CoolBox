package io.coolbox.synthetic;
import com.sun.jna.Library; import io.coolbox.internal.NativeLibraries; import com.sun.jna.Pointer;
interface SyntheticNative extends Library {
 SyntheticNative INSTANCE=NativeLibraries.bridge(SyntheticNative.class);
 int coolbox_misc_make_regression(long n,long f,double noise,double bias,long seed,double[] x,long xc,double[] y,long yc,Pointer err,long cap);
 int coolbox_misc_make_classification(long n,long f,long classes,double sep,long seed,double[] x,long xc,long[] labels,long lc,Pointer err,long cap);
 int coolbox_misc_make_blobs(long n,long centers,long f,double std,long seed,double[] x,long xc,long[] labels,long lc,Pointer err,long cap);
}
