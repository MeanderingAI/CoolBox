package io.coolbox.finance;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class FinanceTest {@Test void evaluatesAndBuildsPaths(){assertEquals(.39894228,Finance.evaluate(Formula.STANDARD_NORMAL_PDF,0),1e-6);assertArrayEquals(new double[]{0,1,2},Finance.brownianMotionPath(new double[]{1,1},new double[]{1,1}),1e-12);}@Test void customPayoffCallback(){MonteCarloEstimate e=Finance.monteCarloEstimateCustomPayoff(100,0,.2,new double[]{.25,.25},new double[][]{{0,0},{0,0}},path->path[path.length-1],1.96);assertEquals(100*Math.exp(-.01),e.price(),1e-9);}@Test void computesGreeks(){double[][] paths={{0},{0},{0},{0}};MonteCarloGreeks g=Finance.monteCarloGreeksFd(MonteCarloGreekContract.EUROPEAN_CALL,100,100,0,.01,.2,new double[]{.5},paths,1.96,.01,.01);assertTrue(g.delta().sampleCount()>0);}@Test void validatesPaths(){assertThrows(IllegalArgumentException.class,()->Finance.brownianMotionPath(new double[]{1},new double[0]));}}


