package coolbox.graphics;

public class ParametricPlot implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public ParametricPlot(int width, int height) {
        this.handle = create(width, height);
    }
    public void setEquations(String xExpr, String yExpr) { setEquations(handle, xExpr, yExpr); }
    public void setTRange(double tMin, double tMax) { setTRange(handle, tMin, tMax); }
    public void setSamples(int n) { setSamples(handle, n); }
    public void setColor(int r, int g, int b, int a) { setColor(handle, r, g, b, a); }
    public Canvas render() { return new Canvas(render(handle)); }
    @Override public void close() { if (handle != 0) { free(handle); handle = 0; } }
    private static native long create(int width, int height);
    private static native void setEquations(long handle, String xExpr, String yExpr);
    private static native void setTRange(long handle, double tMin, double tMax);
    private static native void setSamples(long handle, int n);
    private static native void setColor(long handle, int r, int g, int b, int a);
    private static native long render(long handle);
    private static native void free(long handle);
}
