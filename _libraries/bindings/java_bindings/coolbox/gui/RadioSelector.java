package coolbox.gui;

public class RadioSelector implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public RadioSelector(String[] options, int selected) {
        this.handle = create(options, selected);
    }
    private static native long create(String[] options, int selected);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
