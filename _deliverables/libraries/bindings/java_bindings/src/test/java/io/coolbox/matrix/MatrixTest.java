package io.coolbox.matrix;
import static org.junit.jupiter.api.Assertions.*; import org.junit.jupiter.api.Test;
class MatrixTest { @Test void vectorsMatricesAndErrors(){try(DenseVector a=new DenseVector(new double[]{1,2});DenseVector b=new DenseVector(new double[]{3,4})){assertEquals(11,a.dot(b));assertArrayEquals(new double[]{4,6},a.add(b).toArray());assertEquals(Math.sqrt(5),a.norm(),1e-12);} try(DenseMatrix a=new DenseMatrix(new double[][]{{1,2},{3,4}});DenseMatrix b=DenseMatrix.identity(2)){assertArrayEquals(new double[][]{{1,2},{3,4}},a.multiply(b).toArray());assertEquals(4,a.at(1,1));assertEquals(3,a.transpose().at(0,1));} assertThrows(IllegalArgumentException.class,()->new DenseMatrix(new double[][]{{1},{2,3}}));} }


