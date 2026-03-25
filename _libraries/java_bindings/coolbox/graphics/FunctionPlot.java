package coolbox.graphics;

public class FunctionPlot implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public FunctionPlot(int width, int height) {
        this.handle = create(width, height);
    }
    public void setEquation(String expr) { setEquation(handle, expr); }
    public void setRange(double xMin, double xMax) { setRange(handle, xMin, xMax); }
    public void setSamples(int n) { setSamples(handle, n); }
    public void setColor(int r, int g, int b, int a) { setColor(handle, r, g, b, a); }
    public Canvas render() { return new Canvas(render(handle)); }
    @Override public void close() { if (handle != 0) { free(handle); handle = 0; } }
    private static native long create(int width, int height);
    private static native void setEquation(long handle, String expr);
    private static native void setRange(long handle, double xMin, double xMax);
    private static native void setSamples(long handle, int n);
    private static native void setColor(long handle, int r, int g, int b, int a);
    private static native long render(long handle);
    private static native void free(long handle);
}
