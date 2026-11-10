package io.coolbox.deeplearning;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import java.util.Arrays;
/** Factories corresponding to Python deep_learning network-template functions. */
public final class DeepLearning {
    private static final DLNative N=DLNative.INSTANCE; private DeepLearning(){}
    private static NeuralNetwork wrap(Pointer p,ErrorBuffer e){return new NeuralNetwork(e.check(p));}
    public static NeuralNetwork binaryClassifier(int inputDim){return binaryClassifier(inputDim,new int[]{64,32});}
    public static NeuralNetwork binaryClassifier(int inputDim,int[] hidden){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_binary_classifier(inputDim,hidden,hidden.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork multiclassClassifier(int inputDim,int classes){return multiclassClassifier(inputDim,classes,new int[]{128,64});}
    public static NeuralNetwork multiclassClassifier(int inputDim,int classes,int[] hidden){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_multiclass_classifier(inputDim,classes,hidden,hidden.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork imageClassifier(int classes){return imageClassifier(classes,3,32,32,"simple");}
    public static NeuralNetwork imageClassifier(int classes,int channels,int height,int width,String architecture){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_image_classifier(classes,channels,height,width,architecture,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork regressor(int inputDim){return regressor(inputDim,1,new int[]{64,32});}
    public static NeuralNetwork regressor(int inputDim,int outputDim,int[] hidden){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_regressor(inputDim,outputDim,hidden,hidden.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork embeddingNetwork(int inputDim,int embeddingDim){return embeddingNetwork(inputDim,embeddingDim,new int[]{128,64});}
    public static NeuralNetwork embeddingNetwork(int inputDim,int embeddingDim,int[] hidden){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_embedding_network(inputDim,embeddingDim,hidden,hidden.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork sequenceClassifier(int inputDim,int classes){return sequenceClassifier(inputDim,classes,128,2);}
    public static NeuralNetwork sequenceClassifier(int inputDim,int classes,int hiddenDim,int layers){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_sequence_classifier(inputDim,classes,hiddenDim,layers,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork simpleAutoencoder(int inputDim,int latentDim){return simpleAutoencoder(inputDim,latentDim,new int[]{128,64});}
    public static NeuralNetwork simpleAutoencoder(int inputDim,int latentDim,int[] hidden){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_simple_autoencoder(inputDim,latentDim,hidden,hidden.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork variationalAutoencoder(int inputDim,int latentDim){return variationalAutoencoder(inputDim,latentDim,new int[]{256,128});}
    public static NeuralNetwork variationalAutoencoder(int inputDim,int latentDim,int[] encoderDims){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_variational_autoencoder(inputDim,latentDim,encoderDims,encoderDims.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork simpleGan(int latentDim,int outputDim){return simpleGan(latentDim,outputDim,new int[]{128,256},new int[]{256,128});}
    public static NeuralNetwork simpleGan(int latentDim,int outputDim,int[] generatorDims,int[] discriminatorDims){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_simple_gan(latentDim,outputDim,generatorDims,generatorDims.length,discriminatorDims,discriminatorDims.length,e.pointer(),e.capacity()),e);}
    public static NeuralNetwork languageModel(int vocabularySize){return languageModel(vocabularySize,512,512,8,6,2048);}
    public static NeuralNetwork languageModel(int vocabularySize,int contextLength,int embeddingDim,int heads,int layers,int feedForwardDim){ErrorBuffer e=new ErrorBuffer();return wrap(N.coolbox_dl_template_language_model(vocabularySize,contextLength,embeddingDim,heads,layers,feedForwardDim,e.pointer(),e.capacity()),e);}
    public static long helixDimension(long actions,long truths,boolean richFeedback,double epsilon,long maxActions){var out=new com.sun.jna.ptr.LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_helix_dimension(actions,truths,richFeedback?1:0,epsilon,maxActions,out,e.pointer(),e.capacity()));return out.getValue();}
}
