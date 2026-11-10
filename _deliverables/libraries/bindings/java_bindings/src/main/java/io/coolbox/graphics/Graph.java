package io.coolbox.graphics;

import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;

/** Native chart builder (Python: {@code graphics.Graph}). */
public final class Graph extends NativeHandle {
    private final int width, height;
    public Graph(int width,int height,GraphType type) { super(create(width,height,type),GraphicsNative.INSTANCE::coolbox_graph_free); this.width=width; this.height=height; }
    public Graph(int width,int height) { this(width,height,GraphType.LINE); }
    private static Pointer create(int w,int h,GraphType t) { if(w<=0||h<=0) throw new IllegalArgumentException("graph dimensions must be positive"); ErrorBuffer e=new ErrorBuffer(); return e.check(GraphicsNative.INSTANCE.coolbox_graph_new(w,h,t.ordinal(),e.pointer(),e.capacity())); }
    public Graph setTitle(String value) { ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_graph_set_title(handle(),value,e.pointer(),e.capacity())); return this; }
    public Graph setXLabel(String value) { ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_graph_set_x_label(handle(),value,e.pointer(),e.capacity())); return this; }
    public Graph setYLabel(String value) { ErrorBuffer e=new ErrorBuffer(); e.check(GraphicsNative.INSTANCE.coolbox_graph_set_y_label(handle(),value,e.pointer(),e.capacity())); return this; }
    public Graph addSeries(Series s) { if(s.x().length==0||s.x().length!=s.y().length) throw new IllegalArgumentException("series x and y values must have equal, non-zero lengths"); ErrorBuffer e=new ErrorBuffer(); Color c=s.color(); e.check(GraphicsNative.INSTANCE.coolbox_graph_add_series(handle(),s.label(),s.x(),s.y(),s.x().length,c.red8(),c.green8(),c.blue8(),c.alpha8(),e.pointer(),e.capacity())); return this; }
    public Canvas render() { ErrorBuffer e=new ErrorBuffer(); return new Canvas(e.check(GraphicsNative.INSTANCE.coolbox_graph_render(handle(),e.pointer(),e.capacity()))); }
    public void saveBMP(String path) { try(Canvas c=render()) { c.saveBMP(path); } }
    public void savePNG(String path) { try(Canvas c=render()) { c.savePNG(path); } }
    public void saveJPG(String path,int quality) { try(Canvas c=render()) { c.saveJPG(path,quality); } }
    public int width() { return width; } public int height() { return height; }
}
