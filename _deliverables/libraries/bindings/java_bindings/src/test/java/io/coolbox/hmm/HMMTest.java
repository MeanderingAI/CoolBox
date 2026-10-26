package io.coolbox.hmm;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class HMMTest {
    @Test void computesSequenceLikelihoodAndViterbiPath() {
        try(var hmm=new HMM(2,2)) {
            hmm.setInitialProbabilities(new double[]{0.6,0.4}).setTransitionMatrix(new double[][]{{0.7,0.3},{0.4,0.6}}).setEmissionMatrix(new double[][]{{0.5,0.5},{0.1,0.9}});
            int[] observations={0,1,0};
            assertTrue(Double.isFinite(hmm.logLikelihood(observations)));
            assertEquals(3,hmm.mostLikelyStates(observations).length);
            assertArrayEquals(new double[]{0.6,0.4},hmm.initialProbabilities(),1e-9);
            hmm.train(new int[][]{{0,1,0},{0,0,1}},10,1e-5,1e-8,3);
        }
    }
    @Test void rejectsBadObservation() { try(var hmm=new HMM(2,2)) { assertThrows(CoolBoxException.class,()->hmm.mostLikelyStates(new int[]{9})); } }
}
