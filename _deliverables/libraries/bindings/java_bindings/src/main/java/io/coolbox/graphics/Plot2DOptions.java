package io.coolbox.graphics;
/** Options for a single-series 2D chart (Python: {@code graphics.Plot2DOptions}). */
public record Plot2DOptions(int width,int height,String title,String xLabel,String yLabel,String seriesLabel,Color color,GraphType type,boolean showWindow,String savePath) {
 public Plot2DOptions { if(width==0)width=800;if(height==0)height=600;if(xLabel==null||xLabel.isEmpty())xLabel="X";if(yLabel==null||yLabel.isEmpty())yLabel="Y";if(seriesLabel==null||seriesLabel.isEmpty())seriesLabel="Series";if(color==null)color=Color.BLUE;if(type==null)type=GraphType.LINE;if(savePath==null||savePath.isEmpty())savePath="plot.png";if(title==null)title=""; }
 public static Plot2DOptions defaults(){return new Plot2DOptions(800,600,"","X","Y","Series",Color.BLUE,GraphType.LINE,false,"plot.png");}
}
