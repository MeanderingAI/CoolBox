package io.coolbox.bayesiannetwork;

import java.util.List;

/** Bayesian-network variable description (Python: {@code ml_core.bayesian_network.Node}). */
public record Node(String name, List<String> states, int index) {
    public Node {
        states = List.copyOf(states);
    }
}
