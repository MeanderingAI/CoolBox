package io.coolbox.vision;
import io.coolbox.deeplearning.Layer;import io.coolbox.internal.ErrorBuffer;
/** Base class for convolutional vision layers, interoperable with deep-learning networks. */
abstract class VisionLayer extends Layer {
 VisionLayer(int kind,int[] params,float eps,float momentum){super(create(kind,params,eps,momentum));}
 private static com.sun.jna.Pointer create(int k,int[] p,float e,float m){ErrorBuffer err=new ErrorBuffer();return err.check(VisionNative.INSTANCE.coolbox_cv_layer_new(k,p,p.length,e,m,err.pointer(),err.capacity()));}
 int property(int p){return VisionNative.INSTANCE.coolbox_cv_layer_property(handle(),p);}
}
