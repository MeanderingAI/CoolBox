package io.coolbox.timeseries;
import com.sun.jna.Pointer; import com.sun.jna.ptr.DoubleByReference; import com.sun.jna.ptr.LongByReference; import io.coolbox.internal.ErrorBuffer; import io.coolbox.internal.Matrices;
/** Time-series functions (Python: {@code time_series} module functions). */
public final class TimeSeriesFunctions {
 private static final TimeseriesNative N=TimeseriesNative.INSTANCE; private TimeSeriesFunctions(){}
 public record SeasonalDecomposition(double[] trend,double[] seasonal,double[] residual){}
 public static SeasonalDecomposition seasonalDecompose(TimeSeries ts,int period){double[][] parts=new double[3][ts.size()];ErrorBuffer e=new ErrorBuffer();for(int i=0;i<3;i++)e.check(N.coolbox_timeseries_decomposition(ts.handle(),period,i,parts[i],parts[i].length,e.pointer(),e.capacity()));return new SeasonalDecomposition(parts[0],parts[1],parts[2]);}
 public static double[] detectOutliersZscore(TimeSeries ts){return outliers(ts,0,3.0);} public static double[] detectOutliersZscore(TimeSeries ts,double threshold){return outliers(ts,0,threshold);} public static double[] detectOutliersIqr(TimeSeries ts){return outliers(ts,1,1.5);} public static double[] detectOutliersIqr(TimeSeries ts,double multiplier){return outliers(ts,1,multiplier);}
 private static double[] outliers(TimeSeries ts,int method,double v){LongByReference count=new LongByReference();ErrorBuffer e=new ErrorBuffer();Pointer p=e.check(N.coolbox_timeseries_outliers(ts.handle(),method,v,count,e.pointer(),e.capacity()));return Matrices.takeDoubles(p,count.getValue());}
 public static TimeSeries interpolateMissing(TimeSeries ts,long... indices){ErrorBuffer e=new ErrorBuffer();return new TimeSeries(e.check(N.coolbox_timeseries_interpolate(ts.handle(),indices,indices.length,e.pointer(),e.capacity())));}
}
