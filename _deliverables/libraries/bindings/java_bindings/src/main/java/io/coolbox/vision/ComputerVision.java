package io.coolbox.vision;
import io.coolbox.deeplearning.Tensor;
/** Static computer-vision functions (Python: {@code computer_vision}). */
public final class ComputerVision {
 private ComputerVision(){}
 public static TransformPipeline createImagenetPipeline(){return TransformPipeline.imagenet();}
 public static TransformPipeline createImagenetPipeline(int imageSize){return TransformPipeline.imagenet(imageSize);}
 public static TransformPipeline createTrainingAugmentationPipeline(int imageSize){return TransformPipeline.trainingAugmentation(imageSize);}
 public static TransformPipeline createTrainingAugmentationPipeline(int imageSize,boolean randomFlip,boolean randomRotation,boolean randomBrightness,boolean randomContrast){return TransformPipeline.trainingAugmentation(imageSize,randomFlip,randomRotation,randomBrightness,randomContrast);}
 public static TransformPipeline createInferencePipeline(int imageSize){return TransformPipeline.inference(imageSize);}
 public static TransformPipeline createInferencePipeline(int imageSize,float[] mean,float[] std){return TransformPipeline.inference(imageSize,mean,std);}
 public static Tensor imageToTensor(Image image){return TransformPipeline.imageToTensor(image);}
 public static Image tensorToImage(Tensor tensor){return TransformPipeline.tensorToImage(tensor);}
 public static Image tensorToImage(Tensor tensor,ImageFormat format){return TransformPipeline.tensorToImage(tensor,format);}
}
