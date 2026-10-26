package io.coolbox.pde;
import org.junit.jupiter.api.Test;import static org.junit.jupiter.api.Assertions.*;
class PdeTest {@Test void accessesDenseMatrixAndSolvesHeat(){try(DenseMatrix m=new DenseMatrix(new double[][]{{0},{1},{0}})){assertEquals(3,m.rows());m.set(1,0,.8);assertEquals(.8,m.get(1,0));assertEquals(.8,m.toArray()[1][0]);assertEquals(3,m.solveHeatEq(.1,1,.01,1).length);}}@Test void checksIndex(){try(DenseMatrix m=new DenseMatrix(2,2)){assertThrows(RuntimeException.class,()->m.get(2,0));}}}
