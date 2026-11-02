package io.coolbox.vision;
import io.coolbox.internal.ErrorBuffer;
/** 2D batch normalization (Python: {@code computer_vision.BatchNorm2DLayer}). */
public final class BatchNorm2DLayer extends VisionLayer {
 public BatchNorm2DLayer(int features){this(features,1e-5f,.1f);}public BatchNorm2DLayer(int features,float epsilon,float momentum){super(3,new int[]{features},epsilon,momentum);}
 public void setTraining(boolean training){ErrorBuffer e=new ErrorBuffer();e.check(VisionNative.INSTANCE.coolbox_cv_layer_set_training(handle(),training?1:0,e.pointer(),e.capacity()));}public boolean isTraining(){return VisionNative.INSTANCE.coolbox_cv_layer_is_training(handle())!=0;}
}
