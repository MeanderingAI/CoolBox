package io.coolbox.vision;
import com.sun.jna.Pointer;import com.sun.jna.ptr.FloatByReference;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.NativeHandle;
/** Image tensor (Python: {@code computer_vision.Image}). */
public final class Image extends NativeHandle {
 private static final VisionNative N=VisionNative.INSTANCE;
 public Image(){super(createDefault(),N::coolbox_cv_image_free);}
 private static Pointer createDefault(){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_cv_image_default_new(e.pointer(),e.capacity()));}
 public Image(int height,int width){this(height,width,ImageFormat.RGB);}
 public Image(int height,int width,ImageFormat format){this(create(height,width,format));}
 public Image(int height,int width,int channels,float[] data){this(create(height,width,channels,data));}
 Image(Pointer p){super(p,N::coolbox_cv_image_free);}
 private static Pointer create(int h,int w,ImageFormat f){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_cv_image_new(h,w,f.value,e.pointer(),e.capacity()));}
 private static Pointer create(int h,int w,int ch,float[] data){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_cv_image_new_data(h,w,ch,data,data.length,e.pointer(),e.capacity()));}
 public int height(){return N.coolbox_cv_image_height(handle());}public int width(){return N.coolbox_cv_image_width(handle());}public int channels(){return N.coolbox_cv_image_channels(handle());}public ImageFormat format(){return ImageFormat.values()[N.coolbox_cv_image_format(handle())];}public long size(){return N.coolbox_cv_image_size(handle());}
 public float[] data(){float[] d=new float[Math.toIntExact(size())];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_data(handle(),d,d.length,e.pointer(),e.capacity()));return d;}
 public float at(int row,int col){return at(row,col,0);}public float at(int row,int col,int channel){var x=new FloatByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_at(handle(),row,col,channel,x,e.pointer(),e.capacity()));return x.getValue();}
 public Image set(int row,int col,int channel,float value){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_set(handle(),row,col,channel,value,e.pointer(),e.capacity()));return this;}
 public Image fill(float value){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_fill(handle(),value,e.pointer(),e.capacity()));return this;}public Image fill(float[] values){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_fill_channels(handle(),values,values.length,e.pointer(),e.capacity()));return this;}
 public Image cloneImage(){ErrorBuffer e=new ErrorBuffer();return new Image(e.check(N.coolbox_cv_image_clone(handle(),e.pointer(),e.capacity())));}public Image toGrayscale(){ErrorBuffer e=new ErrorBuffer();return new Image(e.check(N.coolbox_cv_image_grayscale(handle(),e.pointer(),e.capacity())));}public Image toRgb(){ErrorBuffer e=new ErrorBuffer();return new Image(e.check(N.coolbox_cv_image_rgb(handle(),e.pointer(),e.capacity())));}
 public float[] mean(){float[] v=new float[channels()];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_mean(handle(),v,v.length,e.pointer(),e.capacity()));return v;}public float[] standardDeviation(){float[] v=new float[channels()];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_std(handle(),v,v.length,e.pointer(),e.capacity()));return v;}public float[] minMax(){var lo=new FloatByReference();var hi=new FloatByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_image_min_max(handle(),lo,hi,e.pointer(),e.capacity()));return new float[]{lo.getValue(),hi.getValue()};}
}

