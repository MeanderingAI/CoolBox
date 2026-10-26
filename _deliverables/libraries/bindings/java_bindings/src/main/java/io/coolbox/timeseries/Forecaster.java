package io.coolbox.timeseries;
import com.sun.jna.Pointer; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.NativeHandle;
abstract class Forecaster extends NativeHandle {
 static final TimeseriesNative N=TimeseriesNative.INSTANCE;
 Forecaster(int kind,int a,int b,int c,double alpha,double beta,double gamma){super(N.coolbox_forecaster_new(kind,a,b,c,alpha,beta,gamma),N::coolbox_forecaster_free);}
 public Forecaster fit(double[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_forecaster_fit(handle(),values,values.length,e.pointer(),e.capacity()));return this;}
 public double[] forecast(int steps){double[] v=new double[steps];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_forecaster_forecast(handle(),steps,v,steps,e.pointer(),e.capacity()));return v;}
}
