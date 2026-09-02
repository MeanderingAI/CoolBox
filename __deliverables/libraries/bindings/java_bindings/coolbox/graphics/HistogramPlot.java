package coolbox.graphics;

public class HistogramPlot implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public HistogramPlot(int width, int height) {
        this.handle = create(width, height);
    }
    public void setData(double[] values) { setData(handle, values); }
    public void setBins(int n) { setBins(handle, n); }
    public void setColor(int r, int g, int b, int a) { setColor(handle, r, g, b, a); }
    public Canvas render() { return new Canvas(render(handle)); }
    @Override public void close() { if (handle != 0) { free(handle); handle = 0; } }
    private static native long create(int width, int height);
    private static native void setData(long handle, double[] values);
    private static native void setBins(long handle, int n);
    private static native void setColor(long handle, int r, int g, int b, int a);
    private static native long render(long handle);
    private static native void free(long handle);
}
