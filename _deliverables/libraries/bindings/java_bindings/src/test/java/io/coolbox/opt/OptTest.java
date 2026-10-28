package io.coolbox.opt;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class OptTest {
 @Test void averagesAndCallbackOptimizer(){assertArrayEquals(new double[]{2,3},Owa.naiveAverageParameters(new double[][]{{1,2},{3,4}}));Optimizer o=new Optimizer(OptimizerType.GENETIC_SEARCH,1,10,8,2,.8,.1,.2,-2,2,4);try{o.optimize(x->-x[0]*x[0],new double[]{.5});assertTrue(Double.isFinite(o.bestScore()));}finally{o.close();}}
 @Test void weightedAndValidationAverages(){assertArrayEquals(new double[]{2.5,4.5},Owa.weightedTensorAverage(new double[][]{{1,3},{3,5}},new double[]{.25,.75},2),1e-9);OwaConfig config=new OwaConfig().mergeIterations(10);OwaResult result=Owa.optimalWeightedAverageLinearRegression(new double[][]{{0},{1}},new double[][]{{1},{2}},new double[]{1,2},config);assertEquals(2,result.modelWeights().length);}
 @Test void rejectsRaggedInput(){assertThrows(IllegalArgumentException.class,()->Owa.naiveAverageParameters(new double[][]{{1},{1,2}}));}
}

