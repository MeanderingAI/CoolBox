package io.coolbox.deeplearning;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.nio.charset.StandardCharsets;
/** Neural network layer (Python: {@code deep_learning.Layer}). */
public class Layer extends NativeHandle {
    static final DLNative N=DLNative.INSTANCE;
    protected Layer(Pointer p){super(p,N::coolbox_dl_layer_free);}
    static Pointer create(int kind,long[] params,double rate){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_layer_new(kind,params,params.length,rate,0,e.pointer(),e.capacity()));}
    public Tensor forward(Tensor input){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_layer_forward(handle(),input.handle(),e.pointer(),e.capacity())));}
    public Tensor backward(Tensor gradient){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_layer_backward(handle(),gradient.handle(),e.pointer(),e.capacity())));}
    public String name(){byte[] b=new byte[128];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_layer_name(handle(),b,b.length,e.pointer(),e.capacity()));return NativeString.read(b);}
    public boolean hasParameters(){return N.coolbox_dl_layer_has_parameters(handle())!=0;}
    public void updateParameters(double learningRate){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_layer_update(handle(),learningRate,e.pointer(),e.capacity()));}
    public void setTraining(boolean training){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_layer_set_training(handle(),training?1:0,e.pointer(),e.capacity()));}
    static final class NativeString {static String read(byte[] b){int n=0;while(n<b.length&&b[n]!=0)n++;return new String(b,0,n,StandardCharsets.UTF_8);}}
}
