package io.coolbox.timeseries;
/** ARMA model (Python: {@code time_series.ARMAModel}). */
public final class ARMAModel extends Forecaster { public ARMAModel(int p,int q){super(3,p,q,0,0,0,0);} public ARMAModel fit(TimeSeries ts){super.fit(ts.values());return this;} }
