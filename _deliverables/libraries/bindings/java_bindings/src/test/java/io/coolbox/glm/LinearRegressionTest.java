package io.coolbox.glm;

import io.coolbox.CoolBoxException;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

class LinearRegressionTest {
    @Test
    void closedFormRecoversLine() {
        double[][] x = {{0}, {1}, {2}, {3}, {4}};
        double[] y = {1, 3, 5, 7, 9};
        try (LinearRegression model = new LinearRegression(100, 0.01, FitMethod.CLOSED_FORM)) {
            model.fit(x, y);
            assertArrayEquals(new double[] {2.0}, model.coefficients(), 1e-6);
            assertEquals(1.0, model.intercept(), 1e-6);
            assertEquals(11.0, model.predict(new double[] {5}), 1e-6);
        }
    }

    @Test
    void predictBeforeFitRaises() {
        try (LinearRegression model = new LinearRegression()) {
            CoolBoxException error = assertThrows(CoolBoxException.class, () -> model.predict(new double[] {1}));
            assertTrue(error.getMessage().contains("not been fitted"));
        }
    }

    @Test
    void closedObjectRejectsUse() {
        LinearRegression model = new LinearRegression();
        model.close();
        model.close();
        assertTrue(model.isClosed());
        assertThrows(IllegalStateException.class, () -> model.predict(new double[] {1}));
    }
}
