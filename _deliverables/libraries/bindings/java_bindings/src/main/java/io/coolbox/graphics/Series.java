package io.coolbox.graphics;

/** Labeled x/y points (Python: {@code graphics.Series}). */
public record Series(String label, double[] x, double[] y, Color color) {
    public Series { if (label == null) label = ""; if (color == null) color = Color.BLUE; }
    public Series(String label, double[] x, double[] y) { this(label, x, y, Color.BLUE); }
}
