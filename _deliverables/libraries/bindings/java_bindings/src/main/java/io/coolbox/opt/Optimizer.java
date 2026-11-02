package io.coolbox.opt;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
import java.util.Arrays;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.ToDoubleFunction;

/** Native optimizer (Python: {@code opt.Optimizer}). */
public final class Optimizer extends NativeHandle {
    private static final OptNative N=OptNative.INSTANCE;
    private static final AtomicLong IDS=new AtomicLong(1);
    private static final ConcurrentHashMap<Long,ToDoubleFunction<double[]>> CALLBACKS=new ConcurrentHashMap<>();
    private final OptimizerType type;
    private volatile RuntimeException callbackFailure;
    private final OptNative.Objective callback=(id,x,n)->{ try { ToDoubleFunction<double[]> f=CALLBACKS.get(Pointer.nativeValue(id)); return f.applyAsDouble(x.getDoubleArray(0,(int)n)); } catch(Throwable e){ callbackFailure=e instanceof RuntimeException?(RuntimeException)e:new RuntimeException(e); return Double.NaN; } };
    public Optimizer(OptimizerType type){this(type,new double[0]);}
    public Optimizer(OptimizerType type,double... config){super(create(type,config),N::coolbox_optimizer_free);this.type=type;}
    private static Pointer create(OptimizerType t,double[] c){ if(t==null)throw new NullPointerException("type"); ErrorBuffer e=new ErrorBuffer(); return e.check(N.coolbox_optimizer_create(t.value,c,c.length,e.pointer(),e.capacity())); }
    public double[] optimize(ToDoubleFunction<double[]> objective,double[] initial){ long id=IDS.getAndIncrement(); callbackFailure=null; CALLBACKS.put(id,objective); try {ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_optimizer_run(handle(),callback,Pointer.createConstant(id),initial,initial.length,e.pointer(),e.capacity()));if(callbackFailure!=null)throw callbackFailure;double[] out=new double[(int)N.coolbox_optimizer_solution_size(handle())];e.check(N.coolbox_optimizer_copy_solution(handle(),out,out.length,e.pointer(),e.capacity()));return out;} finally { CALLBACKS.remove(id); } }
    public double[] optimize(ToDoubleFunction<double[]> objective){return optimize(objective,new double[0]);}
    public double bestScore(){return N.coolbox_optimizer_best_score(handle());}
    public double bestProbability(){return N.coolbox_optimizer_best_probability(handle());}
    public double[] quantumBounds(){double[] o=new double[5];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_optimizer_quantum_bounds(handle(),o,o.length,e.pointer(),e.capacity()));return o;}
    public double[] transformFrequencyPoint(double[] point,boolean unwarp){double[] o=new double[point.length];ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_optimizer_warp_point(handle(),point,point.length,o,unwarp?1:0,e.pointer(),e.capacity()));return o;}
}

