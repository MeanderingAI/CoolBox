package io.coolbox.graphics;

/** RGBA color (Python: {@code graphics.Color}). */
public record Color(int r, int g, int b, int a) {
    public static final Color BLACK = new Color(0, 0, 0, 255), WHITE = new Color(255, 255, 255, 255), RED = new Color(220, 50, 50, 255), GREEN = new Color(50, 180, 50, 255), BLUE = new Color(50, 90, 220, 255), ORANGE = new Color(230, 150, 30, 255), PURPLE = new Color(150, 50, 200, 255), CYAN = new Color(50, 200, 200, 255), GRAY = new Color(180, 180, 180, 255), DARK_GRAY = new Color(100, 100, 100, 255), LIGHT_GRAY = new Color(230, 230, 230, 255);
    public Color { if ((r | g | b | a) < 0 || r > 255 || g > 255 || b > 255 || a > 255) throw new IllegalArgumentException("color channels must be in [0,255]"); }
    byte red8() { return (byte) r; } byte green8() { return (byte) g; } byte blue8() { return (byte) b; } byte alpha8() { return (byte) a; }
}
