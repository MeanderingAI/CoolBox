package io.coolbox.graphics;

import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;

/** Rendered RGBA chart canvas (Python: {@code graphics.Canvas}). */
public final class Canvas extends NativeHandle {
    Canvas(com.sun.jna.Pointer pointer) { super(pointer,GraphicsNative.INSTANCE::coolbox_canvas_free); }
    public int width() { return GraphicsNative.INSTANCE.coolbox_canvas_width(handle()); }
    public int height() { return GraphicsNative.INSTANCE.coolbox_canvas_height(handle()); }
    public byte[] rgba() { int w=width(),h=height(); long size=GraphicsNative.INSTANCE.coolbox_canvas_data_size(handle()); if(w<=0||h<=0||size>Integer.MAX_VALUE) throw new IllegalStateException("invalid native canvas dimensions"); byte[] data=new byte[(int)size]; ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_canvas_copy_data(handle(),data,data.length,e.pointer(),e.capacity())); return data; }
    public void saveBMP(String path) { save(path,0,0); } public void savePNG(String path) { save(path,1,0); } public void saveJPG(String path,int quality) { save(path,2,quality); }
    private void save(String path,int format,int quality) { ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_canvas_save(handle(),path,format,quality,e.pointer(),e.capacity())); }
    public void showWindow(String title,String savePath) { ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_canvas_show_window(handle(),title,savePath,e.pointer(),e.capacity())); }
}
