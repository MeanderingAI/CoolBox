package io.coolbox.electronics;
/** Circuit output from Python {@code py_circuitry.CircuitSolution}. */
public record CircuitSolution(java.util.Map<Integer,Double> nodeVoltages,java.util.List<ComponentResult> componentResults) {}
