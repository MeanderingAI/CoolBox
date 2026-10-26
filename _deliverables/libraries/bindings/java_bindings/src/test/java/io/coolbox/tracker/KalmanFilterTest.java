package io.coolbox.tracker;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class KalmanFilterTest {
    @Test void predictsAndUpdatesState() {
        try(var filter=new KalmanFilter(1,new double[][]{{1,1},{0,1}},new double[][]{{1,0}},new double[][]{{0.01,0},{0,0.01}},new double[][]{{0.1}},new double[][]{{1,0},{0,1}})) {
            filter.initialize(new double[]{0,1}); filter.predict(); assertArrayEquals(new double[]{1,1},filter.state(),1e-9);
            filter.update(new double[]{1.1}); assertEquals(2,filter.state().length); assertEquals(2,filter.covariance().length);
        }
    }
    @Test void closedFilterRejectsUse() {
        var filter=new KalmanFilter(1,new double[][]{{1}},new double[][]{{1}},new double[][]{{1}},new double[][]{{1}},new double[][]{{1}});
        filter.close(); assertThrows(IllegalStateException.class,filter::predict);
    }
}
