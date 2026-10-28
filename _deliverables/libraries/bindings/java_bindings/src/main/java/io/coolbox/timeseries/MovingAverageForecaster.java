package io.coolbox.timeseries;
import com.sun.jna.ptr.DoubleByReference; import io.coolbox.internal.ErrorBuffer;
/** Moving-average forecast model (Python: {@code time_series.MovingAverageForecaster}). */
public final class MovingAverageForecaster extends Forecaster {
 public MovingAverageForecaster(int windowSize){super(0,windowSize,0,0,0,0,0);}
 public MovingAverageForecaster fit(TimeSeries ts){super.fit(ts.values());return this;}
 public double forecastOneStep(){DoubleByReference out=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_moving_average_forecast_one_step(handle(),out,e.pointer(),e.capacity()));return out.getValue();}
}
