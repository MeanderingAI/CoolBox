package coolbox.gui;

public class LayerList implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public LayerList(String[] layers, int selected) {
        this.handle = create(layers, selected);
    }
    private static native long create(String[] layers, int selected);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
