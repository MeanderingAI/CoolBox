package coolbox.gui;

public class DockPanel implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public DockPanel(String title, boolean floating) {
        this.handle = create(title, floating);
    }
    private static native long create(String title, boolean floating);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
