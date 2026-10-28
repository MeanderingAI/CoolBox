package io.coolbox.graphics;
import io.coolbox.internal.ErrorBuffer;
/** Static chart helpers (Python: {@code graphics.Plot2D} / {@code graphics.Plot3D}). */
public final class Graphics {
 private Graphics(){}
 public static Canvas plot2D(double[] x,double[] y){return plot2D(x,y,Plot2DOptions.defaults());}
 public static Canvas plot2D(double[] x,double[] y,Plot2DOptions o){if(x.length==0||x.length!=y.length)throw new IllegalArgumentException("plot x and y values must have equal, non-zero lengths");Color c=o.color();ErrorBuffer e=new ErrorBuffer();Canvas canvas=new Canvas(e.check(GraphicsNative.INSTANCE.coolbox_plot_2d(x,y,x.length,o.title(),o.xLabel(),o.yLabel(),o.seriesLabel(),c.red8(),c.green8(),c.blue8(),c.alpha8(),o.width(),o.height(),o.type().ordinal(),e.pointer(),e.capacity())));if(o.showWindow())canvas.showWindow(o.title(),o.savePath());return canvas;}
 public static Canvas plot3D(double[] x,double[] y,double[] z){return plot3D(x,y,z,Plot3DOptions.defaults());}
 public static Canvas plot3D(double[] x,double[] y,double[] z,Plot3DOptions o){if(x.length==0||x.length!=y.length||x.length!=z.length)throw new IllegalArgumentException("plot x, y, and z values must have equal, non-zero lengths");Color c=o.color();ErrorBuffer e=new ErrorBuffer();Canvas canvas=new Canvas(e.check(GraphicsNative.INSTANCE.coolbox_plot_3d(x,y,z,x.length,o.title(),o.xLabel(),o.yLabel(),o.zLabel(),o.seriesLabel(),c.red8(),c.green8(),c.blue8(),c.alpha8(),o.width(),o.height(),o.type().ordinal(),o.azimuthDegrees(),o.elevationDegrees(),e.pointer(),e.capacity())));if(o.showWindow())canvas.showWindow(o.title(),o.savePath());return canvas;}
}
