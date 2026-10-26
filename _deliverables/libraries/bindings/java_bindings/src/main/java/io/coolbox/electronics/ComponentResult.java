package io.coolbox.electronics;
/** Per-component output from Python {@code py_circuitry.ComponentResult}. */
public record ComponentResult(String label,String typeName,double resistance,double voltageDrop,double current,double power,double emf,double internalResistance,double terminalVoltage) {}
