package io.coolbox.timeseries;
import com.sun.jna.Pointer; import com.sun.jna.StringArray; import com.sun.jna.ptr.DoubleByReference; import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices; import io.coolbox.internal.NativeHandle; import java.util.Arrays; import java.util.List;
/** Univariate observations and transformations (Python: {@code time_series.TimeSeries}). */
public final class TimeSeries extends NativeHandle {
 private static final TimeseriesNative N=TimeseriesNative.INSTANCE;
 public TimeSeries(double[] values){this(values,(List<String>)null);}
 public TimeSeries(double[] values,List<String> timestamps){super(create(values,timestamps),N::coolbox_timeseries_free);}
 public TimeSeries(double[] values,String[] timestamps){this(values,timestamps==null?null:Arrays.asList(timestamps));}
 private static Pointer create(double[] v,List<String> ts){StringArray a=ts==null?null:new StringArray(ts.toArray(new String[0]));ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_timeseries_new(v,v.length,a,a==null?0:ts.size(),e.pointer(),e.capacity()));}
 public int size(){return Math.toIntExact(N.coolbox_series_size(handle()));}
 public double[] values(){double[] out=new double[size()];if(N.coolbox_series_copy_values(handle(),out,out.length)==0)throw new IllegalStateException("could not copy series values");return out;}
 public double at(int index){DoubleByReference r=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_timeseries_at(handle(),index,r,e.pointer(),e.capacity()));return r.getValue();}
 public String timestampAt(int index){ErrorBuffer e=new ErrorBuffer();return Matrices.takeString(e.check(N.coolbox_timeseries_timestamp(handle(),index,e.pointer(),e.capacity())));}
 private double stat(int id){DoubleByReference r=new DoubleByReference();if(N.coolbox_series_statistic(handle(),id,r)==0)throw new IllegalStateException("could not calculate time-series statistic");return r.getValue();}
 public double mean(){return stat(0);} public double std(){return stat(1);} public double min(){return stat(2);} public double max(){return stat(3);} public double median(){return stat(4);}
 private TimeSeries transform(int op,long n,double a,double b){ErrorBuffer e=new ErrorBuffer();return new TimeSeries(e.check(N.coolbox_series_transform(handle(),op,n,a,b,e.pointer(),e.capacity())));}
 TimeSeries(Pointer p){super(p,N::coolbox_series_free);}
 public TimeSeries normalize(){return transform(0,0,0,0);} public TimeSeries minMaxScale(){return minMaxScale(0,1);} public TimeSeries minMaxScale(double low,double high){return transform(1,0,low,high);} public TimeSeries diff(){return diff(1);} public TimeSeries diff(int lag){return transform(2,lag,0,0);} public TimeSeries logTransform(){return transform(3,0,0,0);} public TimeSeries movingAverage(int window){return transform(4,window,0,0);} public TimeSeries exponentialSmoothing(double alpha){return transform(5,0,alpha,0);} public TimeSeries resample(int size){return transform(6,size,0,0);}
 public double[] autocorrelation(int maxLag){int count=Math.toIntExact(N.coolbox_series_autocorrelation_size(handle(),maxLag));double[] out=new double[count];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_series_copy_autocorrelation(handle(),maxLag,out,count,e.pointer(),e.capacity()));return out;}
 public double[][] createWindows(int window){return createWindows(window,1);} public double[][] createWindows(int window,int stride){ErrorBuffer e=new ErrorBuffer();int rows=Math.toIntExact(N.coolbox_timeseries_window_count(handle(),window,stride,e.pointer(),e.capacity()));e.throwIfSet();double[] out=new double[rows*window];e.check(N.coolbox_timeseries_windows(handle(),window,stride,out,out.length,e.pointer(),e.capacity()));return Matrices.unflatten(out,rows,window);}
}

