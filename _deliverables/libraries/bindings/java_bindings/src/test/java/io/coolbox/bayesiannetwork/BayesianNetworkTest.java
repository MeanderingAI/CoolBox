package io.coolbox.bayesiannetwork;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import java.util.Map;
import static org.junit.jupiter.api.Assertions.*;

class BayesianNetworkTest {
    @Test void computesJointAndConditionalProbabilities() {
        try(var network=new BayesianNetwork()) {
            int a=network.addNode("A",java.util.List.of("true","false")); int b=network.addNode("B",java.util.List.of("true","false"));
            network.addEdge(a,b).setCpt(a,new double[][]{{0.6,0.4}}).setCpt(b,new double[][]{{0.8,0.2},{0.1,0.9}});
            assertEquals(0.12,network.calculateJointProbability(Map.of(a,0,b,1)),1e-9);
            assertEquals(0.2,network.infer(b,1,Map.of(a,0)),1e-9);
        }
    }
    @Test void rejectsInvalidNodeIndex() { try(var network=new BayesianNetwork()) { assertThrows(CoolBoxException.class,()->network.addNode(null,java.util.List.of("yes"))); } }
}
