package io.coolbox.deeplearning;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.util.List;
/** Sequential neural network (Python: {@code deep_learning.NeuralNetwork}). */
public final class NeuralNetwork extends NativeHandle {
    private static final DLNative N=DLNative.INSTANCE;
    public NeuralNetwork(){this(create());}
    NeuralNetwork(Pointer p){super(p,N::coolbox_dl_network_free);}
    private static Pointer create(){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_network_new(e.pointer(),e.capacity()));}
    public NeuralNetwork addLayer(Layer layer){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_add_layer(handle(),layer.handle(),e.pointer(),e.capacity()));return this;}
    public NeuralNetwork setLoss(Loss loss){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_set_loss(handle(),loss.handle(),e.pointer(),e.capacity()));return this;}
    public NeuralNetwork setOptimizer(Optimizer optimizer){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_set_optimizer(handle(),optimizer.handle(),e.pointer(),e.capacity()));return this;}
    public Tensor predict(Tensor input){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_network_predict(handle(),input.handle(),e.pointer(),e.capacity())));}
    public double trainStep(Tensor input,Tensor target){var v=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_train_step(handle(),input.handle(),target.handle(),v,e.pointer(),e.capacity()));return v.getValue();}
    public double evaluate(List<Tensor> inputs,List<Tensor> targets){if(inputs.isEmpty()||inputs.size()!=targets.size())throw new IllegalArgumentException("inputs and targets must be non-empty and have equal length");Pointer[] x=handles(inputs),y=handles(targets);var v=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_evaluate(handle(),x,y,x.length,v,e.pointer(),e.capacity()));return v.getValue();}
    public NeuralNetwork train(List<Tensor> inputs,List<Tensor> targets,int epochs){return train(inputs,targets,epochs,32,true);}
    public NeuralNetwork train(List<Tensor> inputs,List<Tensor> targets,int epochs,int batchSize,boolean verbose){if(inputs.isEmpty()||inputs.size()!=targets.size())throw new IllegalArgumentException("inputs and targets must be non-empty and have equal length");Pointer[] x=handles(inputs),y=handles(targets);ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_train(handle(),x,y,x.length,epochs,batchSize,verbose?1:0,e.pointer(),e.capacity()));return this;}
    public NeuralNetwork setTraining(boolean training){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_network_set_training(handle(),training?1:0,e.pointer(),e.capacity()));return this;}
    public long layerCount(){return N.coolbox_dl_network_layer_count(handle());}
    private static Pointer[] handles(List<Tensor> values){return values.stream().map(Tensor::handle).toArray(Pointer[]::new);}
}
