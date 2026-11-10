package io.coolbox.dimred;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class DimredTest {
    @Test void svdAndPcaTransformData() {
        double[][] x={{1,2},{2,4},{3,6}};
        try(var svd=new SVD()) { svd.compute(x); assertTrue(svd.isComputed()); assertEquals(1,svd.rank()); assertEquals(3,svd.reconstruct().length); }
        try(var pca=new PCA(1,true,false)) { double[][] z=pca.fitTransform(x); assertEquals(3,z.length); assertEquals(1,z[0].length); assertEquals(2,pca.components().length); assertEquals(2,pca.mean().length); assertEquals(2,pca.scale().length); assertEquals(1,pca.explainedVariance().length); assertEquals(1,pca.explainedVarianceRatio().length); assertEquals(3,pca.inverseTransform(z).length); }
    }
    @Test void knnFindsNearestNeighbor() {
        double[][] x={{0,0},{0,1},{10,10},{10,11}};
        try(var knn=new KNN(1,"euclidean")) { knn.fit(x); assertEquals(0,knn.kneighbors(new double[][]{{0.1,0}}).indices()[0][0]); assertTrue(knn.isFitted()); assertEquals(4,knn.kneighbors().indices().length); assertEquals(4,knn.pairwiseDistances(x,x).length); assertEquals("euclidean",knn.metric()); }
    }
    @Test void umapEmbedsData() {
        double[][] x={{0,0},{0,1},{10,10},{10,11}};
        try(var umap=new UMAP(2,2,0.1,"euclidean",1.0,1,9)) { double[][] y=umap.fitTransform(x); assertEquals(4,y.length); assertEquals(2,y[0].length); assertTrue(umap.isFitted()); }
    }
    @Test void svdRequiresComputeBeforeRead() { try(var svd=new SVD()) { assertThrows(CoolBoxException.class,svd::getU); } }
}
