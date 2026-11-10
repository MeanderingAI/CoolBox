package io.coolbox.svm;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class SVMTest {
    @Test void kernelsAndClassifierWork() {
        try (var kernel=new LinearKernel();var model=new SVM(kernel)) {
            assertEquals(11.0,kernel.calculate(new double[]{1,2},new double[]{3,4}),1e-9);
            model.fit(new double[][]{{-2},{-1},{1},{2}},new double[]{-1,-1,1,1});
            assertTrue(model.predict(new double[]{1.5})>0);
            assertTrue(model.predict(new double[]{-1.5})<0);
        }
    }
    @Test void predictionBeforeFitFails() { try(var model=new SVM(new RBFKernel(0.5))) { assertThrows(CoolBoxException.class,()->model.predict(new double[]{1})); } }
    @Test void polynomialAndSigmoidConstructorsWork() { try(var p=new PolynomialKernel(2,1,0);var s=new SigmoidKernel(0.5,0.2)) { assertTrue(Double.isFinite(p.calculate(new double[]{1},new double[]{2}))); assertTrue(Double.isFinite(s.calculate(new double[]{1},new double[]{2}))); } }

    @Test void classifierRetainsNativeKernelAfterJavaKernelIsClosed() {
        try (var kernel = new LinearKernel(); var model = new SVM(kernel)) {
            kernel.close();
            assertThrows(IllegalStateException.class, () -> kernel.calculate(new double[] {1}, new double[] {2}));
            model.fit(new double[][] {{-2}, {-1}, {1}, {2}}, new double[] {-1, -1, 1, 1});
            assertTrue(model.predict(new double[] {1.5}) > 0);
            model.close();
            assertThrows(IllegalStateException.class, () -> model.predict(new double[] {1.5}));
        }
    }

    @Test void mismatchedNativeDimensionsAreReportedAsJavaExceptions() {
        try (var kernel = new LinearKernel(); var model = new SVM(kernel)) {
            assertThrows(CoolBoxException.class,
                    () -> kernel.calculate(new double[] {1}, new double[] {1, 2}));
            model.fit(new double[][] {{-2}, {-1}, {1}, {2}}, new double[] {-1, -1, 1, 1});
            assertThrows(CoolBoxException.class, () -> model.predict(new double[] {1, 2}));
        }
    }
}
