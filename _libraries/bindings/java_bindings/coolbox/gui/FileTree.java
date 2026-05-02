package coolbox.gui;

public class FileTree implements AutoCloseable {
    private long handle;
    static {
        System.loadLibrary("GuiComponentsBridge");
    }
    public FileTree(String rootName) {
        this.handle = create(rootName);
    }
    private static native long create(String rootName);
    private static native void free(long handle);
    @Override
    public void close() {
        if (handle != 0) {
            free(handle);
            handle = 0;
        }
    }
}
