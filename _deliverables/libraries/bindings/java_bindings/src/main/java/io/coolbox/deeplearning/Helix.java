package io.coolbox.deeplearning;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.NativeHandle;
/** HELiX active-learning agent (Python: {@code deep_learning.agents.Helix}). */
public final class Helix extends NativeHandle {
    private static final DLNative N=DLNative.INSTANCE;
    public Helix(long actions,long truth,boolean richFeedback){super(create(actions,truth,richFeedback),N::coolbox_dl_helix_free);}
    private static com.sun.jna.Pointer create(long a,long t,boolean r){ErrorBuffer e=new ErrorBuffer();return e.check(N.coolbox_dl_helix_new(a,t,r?1:0,e.pointer(),e.capacity()));}
    public long selectAction(){var v=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_helix_select_action(handle(),v,e.pointer(),e.capacity()));return v.getValue();}
    public Helix observe(long action,String feedback){ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_helix_observe(handle(),action,feedback,e.pointer(),e.capacity()));return this;}
    public long interact(){var v=new LongByReference();ErrorBuffer e=new ErrorBuffer();e.check(N.coolbox_dl_helix_interact(handle(),v,e.pointer(),e.capacity()));return v.getValue();}
    public long rounds(){return N.coolbox_dl_helix_rounds(handle());} public long mistakes(){return N.coolbox_dl_helix_mistakes(handle());} public double cumulativeRegret(){return N.coolbox_dl_helix_cumulative_regret(handle());} public boolean isIdentified(){return N.coolbox_dl_helix_identified(handle())!=0;}
    public long[] versionSpace(){long n=N.coolbox_dl_helix_version_space(handle(),null,0);long[] out=new long[Math.toIntExact(n)];N.coolbox_dl_helix_version_space(handle(),out,out.length);return out;}
}
