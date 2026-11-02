package io.coolbox.timeseries;
/** ARIMA model (Python: {@code time_series.ARIMAModel}). */
public final class ARIMAModel extends Forecaster { public ARIMAModel(int p,int d,int q){super(4,p,d,q,0,0,0);} public ARIMAModel fit(TimeSeries ts){super.fit(ts.values());return this;} }
