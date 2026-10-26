package io.coolbox.vision;
import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class ComputerVisionTest {
 @Test void transformPipelineAndTensorInterop(){try(Image image=new Image(4,4,ImageFormat.RGB);Resize resize=new Resize(2,2);TransformPipeline pipeline=new TransformPipeline().add(resize)){image.fill(.5f);try(Image scaled=pipeline.apply(image);io.coolbox.deeplearning.Tensor tensor=TransformPipeline.imageToTensor(scaled);Image copy=TransformPipeline.tensorToImage(tensor)){assertEquals(2,copy.height());assertEquals(2,copy.width());assertEquals(3,copy.channels());assertEquals(12,tensor.size());}}}
 @Test void invalidTransformApplicationIsRejected(){try(Image image=new Image(2,2,ImageFormat.RGB);Normalize normalize=new Normalize(new float[]{0f},new float[]{1f})){assertThrows(CoolBoxException.class,()->normalize.apply(image));}}
}
