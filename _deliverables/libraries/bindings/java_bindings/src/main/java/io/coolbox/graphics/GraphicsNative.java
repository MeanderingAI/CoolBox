package io.coolbox.graphics;

import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.internal.NativeLibraries;

interface GraphicsNative extends Library {
    GraphicsNative INSTANCE = NativeLibraries.bridge(GraphicsNative.class);
    Pointer coolbox_graph_new(int width,int height,int type,Pointer error,long cap); void coolbox_graph_free(Pointer graph);
    int coolbox_graph_set_title(Pointer graph,String title,Pointer error,long cap); int coolbox_graph_set_x_label(Pointer graph,String label,Pointer error,long cap); int coolbox_graph_set_y_label(Pointer graph,String label,Pointer error,long cap);
    int coolbox_graph_add_series(Pointer graph,String label,double[] x,double[] y,long count,byte r,byte g,byte b,byte a,Pointer error,long cap);
    Pointer coolbox_graph_render(Pointer graph,Pointer error,long cap);
    Pointer coolbox_plot_2d(double[] x,double[] y,long count,String title,String xlabel,String ylabel,String series,byte r,byte g,byte b,byte a,int width,int height,int type,Pointer error,long cap);
    Pointer coolbox_plot_3d(double[] x,double[] y,double[] z,long count,String title,String xlabel,String ylabel,String zlabel,String series,byte r,byte g,byte b,byte a,int width,int height,int type,double azimuth,double elevation,Pointer error,long cap);
    int coolbox_canvas_show_window(Pointer canvas,String title,String path,Pointer error,long cap); void coolbox_canvas_free(Pointer canvas); int coolbox_canvas_width(Pointer canvas); int coolbox_canvas_height(Pointer canvas); long coolbox_canvas_data_size(Pointer canvas); int coolbox_canvas_copy_data(Pointer canvas,byte[] output,long capacity,Pointer error,long cap); int coolbox_canvas_save(Pointer canvas,String path,int format,int quality,Pointer error,long cap);
}
