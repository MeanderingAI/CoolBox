package io.coolbox.tracker;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;

/** Linear Kalman filter (Python: {@code ml_core.tracker.KalmanFilter}). */
public final class KalmanFilter extends NativeHandle {
    private static final TrackerNative NATIVE=TrackerNative.INSTANCE;
    public KalmanFilter(double dt,double[][] transition,double[][] observation,double[][] processNoise,double[][] measurementNoise,double[][] initialCovariance) {
        super(create(dt,transition,observation,processNoise,measurementNoise,initialCovariance),NATIVE::coolbox_tracker_kalman_free);
    }
    private static Pointer create(double dt,double[][] a,double[][] c,double[][] q,double[][] r,double[][] p) {
        int ac=Matrices.columns(a),cc=Matrices.columns(c),qc=Matrices.columns(q),rc=Matrices.columns(r),pc=Matrices.columns(p); var e=new ErrorBuffer();
        return e.check(NATIVE.coolbox_tracker_kalman_new(dt,Matrices.flatten(a),a.length,ac,Matrices.flatten(c),c.length,cc,Matrices.flatten(q),q.length,qc,Matrices.flatten(r),r.length,rc,Matrices.flatten(p),p.length,pc,e.pointer(),e.capacity()));
    }
    public KalmanFilter initialize(double[] initialState) { var e=new ErrorBuffer();e.check(NATIVE.coolbox_tracker_kalman_init(handle(),initialState,initialState.length,e.pointer(),e.capacity()));return this; }
    public KalmanFilter predict() { var e=new ErrorBuffer();e.check(NATIVE.coolbox_tracker_kalman_predict(handle(),e.pointer(),e.capacity()));return this; }
    public KalmanFilter update(double[] measurement) { var e=new ErrorBuffer();e.check(NATIVE.coolbox_tracker_kalman_update(handle(),measurement,measurement.length,e.pointer(),e.capacity()));return this; }
    public double[] state() { var n=new LongByReference();var e=new ErrorBuffer();Pointer p=NATIVE.coolbox_tracker_kalman_state(handle(),n,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeDoubles(p,n.getValue()); }
    public double[][] covariance() { var r=new LongByReference();var c=new LongByReference();var e=new ErrorBuffer();Pointer p=NATIVE.coolbox_tracker_kalman_covariance(handle(),r,c,e.pointer(),e.capacity());e.throwIfSet();return Matrices.takeMatrix(p,r.getValue(),c.getValue()); }
}
