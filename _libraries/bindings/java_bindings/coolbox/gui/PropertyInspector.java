package coolbox.gui;

public class PropertyInspector implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public PropertyInspector(String[] keys, String[] values) {
        this.handle = create(keys, values);
    }
    private static native long create(String[] keys, String[] values);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
