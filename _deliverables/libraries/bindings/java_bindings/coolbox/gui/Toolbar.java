package coolbox.gui;

public class Toolbar implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public Toolbar(String[] actions) {
        this.handle = create(actions);
    }
    private static native long create(String[] actions);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
