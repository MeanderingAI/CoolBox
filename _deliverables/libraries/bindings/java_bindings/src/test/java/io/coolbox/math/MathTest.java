package io.coolbox.math;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class MathTest {@Test void solvesOneDimensionalHeatEquation(){double[] out=HeatEquation.solve(new double[]{0,1,0},new long[]{3,1,1},1,.1,1,1,0);assertEquals(3,out.length);assertTrue(Double.isFinite(out[1]));}@Test void rejectsMismatchedShape(){assertThrows(IllegalArgumentException.class,()->HeatEquation.solve(new double[]{1},new long[]{3,1,1},1,.1,1,1,0));}}


