package io.coolbox.vision;
import com.sun.jna.Pointer;import io.coolbox.deeplearning.Tensor;import io.coolbox.internal.ErrorBuffer;import io.coolbox.internal.NativeHandle;import java.util.ArrayList;import java.util.List;
/** Ordered image-transform sequence (Python: {@code computer_vision.TransformPipeline}). */
public final class TransformPipeline extends NativeHandle {
 private static final VisionNative N=VisionNative.INSTANCE;
 public TransformPipeline(){this(create());}private TransformPipeline(Pointer p){super(p,N::coolbox_cv_pipeline_free);}
 private static Pointer create(){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_cv_pipeline_new(e.pointer(),e.capacity()));}
 public TransformPipeline add(Transform t){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_cv_pipeline_add(handle(),t.handle(),e.pointer(),e.capacity()));return this;}
 public Image apply(Image image){ErrorBuffer e=new ErrorBuffer();return new Image(e.check(N.coolbox_cv_pipeline_apply(handle(),image.handle(),e.pointer(),e.capacity())));}
 public List<Image> applyBatch(List<Image> images){List<Image> out=new ArrayList<>(images.size());for(Image image:images)out.add(apply(image));return out;}
 public long size(){return N.coolbox_cv_pipeline_size(handle());}public void clear(){N.coolbox_cv_pipeline_clear(handle());}
 public TransformPipeline clonePipeline(){ErrorBuffer e=new ErrorBuffer();return new TransformPipeline(e.check(N.coolbox_cv_pipeline_clone(handle(),e.pointer(),e.capacity())));}
 public static TransformPipeline imagenet(){return imagenet(224);}public static TransformPipeline imagenet(int imageSize){ErrorBuffer e=new ErrorBuffer();return new TransformPipeline(e.check(N.coolbox_cv_pipeline_imagenet(imageSize,e.pointer(),e.capacity())));}
 public static TransformPipeline trainingAugmentation(int size){return trainingAugmentation(size,true,true,true,true);}public static TransformPipeline trainingAugmentation(int size,boolean flip,boolean rotation,boolean brightness,boolean contrast){ErrorBuffer e=new ErrorBuffer();return new TransformPipeline(e.check(N.coolbox_cv_pipeline_training(size,flip?1:0,rotation?1:0,brightness?1:0,contrast?1:0,e.pointer(),e.capacity())));}
 public static TransformPipeline inference(int size){return inference(size,new float[]{.485f,.456f,.406f},new float[]{.229f,.224f,.225f});}public static TransformPipeline inference(int size,float[] mean,float[] std){if(mean.length!=std.length)throw new IllegalArgumentException("mean and std lengths must match");ErrorBuffer e=new ErrorBuffer();return new TransformPipeline(e.check(N.coolbox_cv_pipeline_inference(size,mean,std,mean.length,e.pointer(),e.capacity())));}
 public static Tensor imageToTensor(Image image){ErrorBuffer e=new ErrorBuffer();return Tensor.adopt(e.check(N.coolbox_cv_image_to_tensor(image.handle(),e.pointer(),e.capacity())));}
 public static Image tensorToImage(Tensor tensor){return tensorToImage(tensor,ImageFormat.RGB);}public static Image tensorToImage(Tensor tensor,ImageFormat format){ErrorBuffer e=new ErrorBuffer();return new Image(e.check(N.coolbox_cv_tensor_to_image(tensor.handle(),format.value,e.pointer(),e.capacity())));}
}
