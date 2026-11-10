package io.coolbox.finance;
import com.sun.jna.Structure;
@Structure.FieldOrder({"price","standard_error","confidence_interval_low","confidence_interval_high","sample_count"})
public class MonteCarloEstimate extends Structure {public double price,standard_error,confidence_interval_low,confidence_interval_high;public long sample_count;public MonteCarloEstimate(){}public double price(){read();return price;}public double standardError(){read();return standard_error;}public double confidenceIntervalLow(){read();return confidence_interval_low;}public double confidenceIntervalHigh(){read();return confidence_interval_high;}public long sampleCount(){read();return sample_count;}}
