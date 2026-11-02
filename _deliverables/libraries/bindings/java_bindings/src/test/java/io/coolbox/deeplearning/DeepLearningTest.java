package io.coolbox.deeplearning;
import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class DeepLearningTest {
 @Test void tensorOperationsAndTemplateNetwork(){try(Tensor a=new Tensor(new long[]{2},new double[]{1,2});Tensor b=a.multiply(2);NeuralNetwork network=DeepLearning.binaryClassifier(2,new int[]{4})){assertArrayEquals(new double[]{2,4},b.data(),1e-9);assertEquals(4,network.layerCount());assertArrayEquals(new long[]{2},a.shape());}}
 @Test void denseParametersAndNamedFunctions(){try(DenseLayer layer=new DenseLayer(2,3);Adam adam=new Adam();MSELoss loss=new MSELoss()){assertEquals(2,layer.weights().length);assertEquals(3,layer.weights()[0].length);assertEquals(3,layer.bias().length);assertEquals("Adam",adam.name());assertEquals("MSE",loss.name());}}
 @Test void invalidTensorShapeIsRejected(){assertThrows(CoolBoxException.class,()->new Tensor(new long[]{2},new double[]{1}));}
}
