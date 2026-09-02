package coolbox.graphics;

public class Fractal implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public Fractal(int width, int height, int type) {
        this.handle = create(width, height, type);
    }
    public void setParams(double param1, double param2) { setParams(handle, param1, param2); }
    public void setMaxIter(int maxIter) { setMaxIter(handle, maxIter); }
    public void setBounds(double xMin, double xMax, double yMin, double yMax) { setBounds(handle, xMin, xMax, yMin, yMax); }
    public Canvas render() { return new Canvas(render(handle)); }
    @Override public void close() { if (handle != 0) { free(handle); handle = 0; } }
    private static native long create(int width, int height, int type);
    private static native void setParams(long handle, double param1, double param2);
    private static native void setMaxIter(long handle, int maxIter);
    private static native void setBounds(long handle, double xMin, double xMax, double yMin, double yMax);
    private static native long render(long handle);
    private static native void free(long handle);
}
