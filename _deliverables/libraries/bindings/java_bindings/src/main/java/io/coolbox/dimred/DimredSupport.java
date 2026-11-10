package io.coolbox.dimred;

import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;

final class DimredSupport {
    private DimredSupport() { }
    static Pointer matrix(Pointer p,LongByReference r,LongByReference c,ErrorBuffer e) { e.throwIfSet(); if(p==null) throw e.failure(); return p; }
    static double[][] takeMatrix(Pointer p,LongByReference r,LongByReference c,ErrorBuffer e) { return Matrices.takeMatrix(matrix(p,r,c,e),r.getValue(),c.getValue()); }
    static double[] takeVector(Pointer p,LongByReference n,ErrorBuffer e) { e.throwIfSet(); if(p==null) throw e.failure(); return Matrices.takeDoubles(p,n.getValue()); }
}
