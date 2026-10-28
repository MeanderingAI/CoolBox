package io.coolbox.timeseries;
/** Exponential-smoothing model (Python: {@code time_series.ExponentialSmoothingForecaster}). */
public final class ExponentialSmoothingForecaster extends Forecaster {
 public ExponentialSmoothingForecaster(double alpha){this(alpha,0,0);} public ExponentialSmoothingForecaster(double alpha,double beta,double gamma){super(1,0,0,0,alpha,beta,gamma);}
 public ExponentialSmoothingForecaster fit(TimeSeries ts){super.fit(ts.values());return this;}
}
