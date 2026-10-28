package io.coolbox.vision;
import com.sun.jna.Pointer;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.NativeHandle;
/** Image transform (Python: {@code computer_vision.Transform}). */
public class Transform extends NativeHandle {
 static final VisionNative N=VisionNative.INSTANCE;
 protected Transform(Pointer p){super(p,N::coolbox_cv_transform_free);}
 static Pointer create(int kind,int[] ip,float[] fp,float[] mean,float[] std,int mode){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_cv_transform_new(kind,ip,ip.length,fp,fp.length,mean,std,mean==null?0:mean.length,mode,e.pointer(),e.capacity()));}
 public Image apply(Image image){ErrorBuffer e=new ErrorBuffer();return new Image(NativeAccess.adopt(N.coolbox_cv_transform_apply(handle(),image.handle(),e.pointer(),e.capacity()),e));}
}
