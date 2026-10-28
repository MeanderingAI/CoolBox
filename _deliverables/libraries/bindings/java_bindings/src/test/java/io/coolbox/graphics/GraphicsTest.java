package io.coolbox.graphics;
import static org.junit.jupiter.api.Assertions.*; import org.junit.jupiter.api.Test;
class GraphicsTest { @Test void chartsRenderAndValidate(){try(Graph g=new Graph(96,64).setTitle("test").setXLabel("x").setYLabel("y").addSeries(new Series("line",new double[]{0,1,2},new double[]{1,2,1}));Canvas c=g.render()){assertEquals(96,c.width());assertEquals(64,c.height());assertEquals(96*64*4,c.rgba().length);} assertThrows(IllegalArgumentException.class,()->Graphics.plot2D(new double[]{1},new double[0]));} }
