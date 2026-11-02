package io.coolbox.timeseries;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.NativeLibraries;
interface TimeseriesNative extends Library {
 TimeseriesNative INSTANCE=NativeLibraries.bridge(TimeseriesNative.class);
 Pointer coolbox_timeseries_new(double[] values,long count,Pointer timestamps,long timestampCount,Pointer error,long errorCapacity);
 void coolbox_timeseries_free(Pointer h); void coolbox_series_free(Pointer h); long coolbox_series_size(Pointer h); int coolbox_series_copy_values(Pointer h,double[] out,long cap);
 int coolbox_series_statistic(Pointer h,int stat,DoubleByReference out);
 Pointer coolbox_series_transform(Pointer h,int op,long integerArg,double a,double b,Pointer err,long cap);
 long coolbox_series_autocorrelation_size(Pointer h,long maxLag); int coolbox_series_copy_autocorrelation(Pointer h,long lag,double[] out,long cap,Pointer err,long capErr);
 int coolbox_timeseries_at(Pointer h,long i,DoubleByReference out,Pointer err,long cap); Pointer coolbox_timeseries_timestamp(Pointer h,long i,Pointer err,long cap);
 long coolbox_timeseries_window_count(Pointer h,long width,long stride,Pointer err,long cap); int coolbox_timeseries_windows(Pointer h,long width,long stride,double[] out,long cap,Pointer err,long errCap);
 int coolbox_timeseries_decomposition(Pointer h,long period,int component,double[] out,long cap,Pointer err,long errCap);
 Pointer coolbox_timeseries_outliers(Pointer h,int method,double param,LongByReference count,Pointer err,long cap);
 Pointer coolbox_timeseries_interpolate(Pointer h,long[] indices,long count,Pointer err,long cap);
 Pointer coolbox_timeseries_ar_new(long order); void coolbox_timeseries_ar_free(Pointer h); int coolbox_timeseries_ar_fit(Pointer h,double[] values,long count,Pointer e,long cap); int coolbox_timeseries_ar_forecast(Pointer h,long steps,double[] out,long capacity,Pointer e,long cap); Pointer coolbox_timeseries_ar_coefficients(Pointer h,com.sun.jna.ptr.LongByReference count,Pointer e,long cap);
 Pointer coolbox_forecaster_new(int kind,long a,long b,long c,double alpha,double beta,double gamma); void coolbox_forecaster_free(Pointer h);
 int coolbox_forecaster_fit(Pointer h,double[] values,long count,Pointer err,long cap); int coolbox_forecaster_forecast(Pointer h,long steps,double[] out,long capacity,Pointer err,long cap);
 int coolbox_moving_average_forecast_one_step(Pointer h,DoubleByReference out,Pointer err,long cap);
 Pointer coolbox_multits_new(double[] data,long features,long samples,Pointer names,long nameCount,Pointer timestamps,long timestampCount,Pointer err,long cap); void coolbox_multits_free(Pointer h);
 long coolbox_multits_features(Pointer h); long coolbox_multits_samples(Pointer h); int coolbox_multits_data(Pointer h,double[] out,long cap,Pointer err,long errCap);
 int coolbox_multits_feature(Pointer h,long i,double[] out,long cap,Pointer err,long errCap); Pointer coolbox_multits_sample(Pointer h,long i,LongByReference count,Pointer err,long cap);
 int coolbox_multits_at(Pointer h,long f,long s,DoubleByReference out,Pointer err,long cap); Pointer coolbox_multits_stats(Pointer h,int stddev,LongByReference count,Pointer err,long cap);
 Pointer coolbox_multits_transform(Pointer h,int scale,Pointer err,long cap); long coolbox_multits_window_count(Pointer h,long width,long stride,Pointer err,long cap);
 int coolbox_multits_windows(Pointer h,long width,long stride,double[] out,long cap,Pointer err,long errCap);
}

