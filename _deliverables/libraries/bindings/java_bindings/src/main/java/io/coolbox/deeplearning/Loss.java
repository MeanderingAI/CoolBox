package io.coolbox.deeplearning;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.DoubleByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
/** Loss-function base (Python: {@code deep_learning.Loss}). */
public class Loss extends NativeHandle {
    static final DLNative N=DLNative.INSTANCE;
    protected Loss(int kind){this(create(kind));}
    protected Loss(Pointer p){super(p,N::coolbox_dl_loss_free);}
    private static Pointer create(int kind){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_loss_new(kind,e.pointer(),e.capacity()));}
    public double compute(Tensor predictions,Tensor targets){var v=new DoubleByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_loss_compute(handle(),predictions.handle(),targets.handle(),v,e.pointer(),e.capacity()));return v.getValue();}
    public String name(){byte[] out=new byte[128];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_loss_name(handle(),out,out.length,e.pointer(),e.capacity()));return Layer.NativeString.read(out);}
    public Tensor gradient(Tensor predictions,Tensor targets){ErrorBuffer e=new ErrorBuffer();return new Tensor(e.check(N.coolbox_dl_loss_gradient(handle(),predictions.handle(),targets.handle(),e.pointer(),e.capacity())));}
}
