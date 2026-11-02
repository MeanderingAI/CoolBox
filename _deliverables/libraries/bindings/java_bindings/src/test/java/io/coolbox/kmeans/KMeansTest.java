package io.coolbox.kmeans;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class KMeansTest {
    @Test void clustersAndExposesFitResults() {
        double[][] x={{0,0},{0,1},{10,10},{10,11}};
        try(var model=new KMeans(2,50,InitMethod.KMEANSPP,7)) {
            int[] labels=model.fitPredict(x); assertEquals(4,labels.length); assertEquals(2,model.centroids().length);
            assertTrue(model.inertia()>=0); assertTrue(model.numberOfIterations()>0); assertEquals(4,model.labels().length);
        }
    }
    @Test void resultsRequireFittedModel() { try(var model=new KMeans()) { assertThrows(CoolBoxException.class,model::labels); assertThrows(CoolBoxException.class,model::numberOfIterations); } }
}
