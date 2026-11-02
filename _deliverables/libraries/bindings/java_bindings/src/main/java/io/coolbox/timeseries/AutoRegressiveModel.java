package io.coolbox.timeseries;
import com.sun.jna.Pointer; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle;
/** Autoregressive model (Python: {@code time_series.AutoRegressiveModel}). */
public final class AutoRegressiveModel extends NativeHandle {
 private static final TimeseriesNative N=TimeseriesNative.INSTANCE;
 public AutoRegressiveModel(int order){super(N.coolbox_timeseries_ar_new(order),N::coolbox_timeseries_ar_free);}
 public AutoRegressiveModel fit(TimeSeries ts){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_timeseries_ar_fit(handle(),ts.values(),ts.size(),e.pointer(),e.capacity()));return this;}
 public double[] forecast(int steps){double[] out=new double[steps];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_timeseries_ar_forecast(handle(),steps,out,steps,e.pointer(),e.capacity()));return out;}
 public double[] coefficients(){LongByReference c=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_timeseries_ar_coefficients(handle(),c,e.pointer(),e.capacity()));return Matrices.takeDoubles(p,c.getValue());}
}
