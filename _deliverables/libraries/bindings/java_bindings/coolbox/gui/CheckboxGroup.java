package coolbox.gui;

public class CheckboxGroup implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public CheckboxGroup(String[] options, boolean[] checked) {
        this.handle = create(options, checked);
    }
    private static native long create(String[] options, boolean[] checked);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
