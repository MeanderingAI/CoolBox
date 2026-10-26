package io.coolbox.finance;
import com.sun.jna.Structure;
@Structure.FieldOrder({"delta","gamma","vega","theta","rho"})
public class MonteCarloGreeks extends Structure {public MonteCarloEstimate delta=new MonteCarloEstimate(),gamma=new MonteCarloEstimate(),vega=new MonteCarloEstimate(),theta=new MonteCarloEstimate(),rho=new MonteCarloEstimate();public MonteCarloGreeks(){}public MonteCarloEstimate delta(){read();return delta;}public MonteCarloEstimate gamma(){read();return gamma;}public MonteCarloEstimate vega(){read();return vega;}public MonteCarloEstimate theta(){read();return theta;}public MonteCarloEstimate rho(){read();return rho;}}
