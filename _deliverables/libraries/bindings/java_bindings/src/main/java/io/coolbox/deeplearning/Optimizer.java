package io.coolbox.deeplearning;
import com.sun.jna.Pointer;
import java.nio.charset.StandardCharsets;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
/** Optimizer base (Python: {@code deep_learning.Optimizer}). */
public class Optimizer extends NativeHandle {
    static final DLNative N=DLNative.INSTANCE;
    protected Optimizer(int kind,double lr,double p1,double p2,double p3){super(create(kind,lr,p1,p2,p3),N::coolbox_dl_optimizer_free);}
    private static Pointer create(int k,double lr,double a,double b,double c){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_optimizer_new_full(k,lr,a,b,c,e.pointer(),e.capacity()));}
    public void step(Tensor parameters,Tensor gradients){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_optimizer_step(handle(),parameters.handle(),gradients.handle(),e.pointer(),e.capacity()));}
    public void reset(){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_optimizer_reset(handle(),e.pointer(),e.capacity()));}
    public String name(){byte[] out=new byte[128];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_optimizer_name(handle(),out,out.length,e.pointer(),e.capacity()));int n=0;while(n<out.length&&out[n]!=0)n++;return new String(out,0,n,StandardCharsets.UTF_8);}
}
