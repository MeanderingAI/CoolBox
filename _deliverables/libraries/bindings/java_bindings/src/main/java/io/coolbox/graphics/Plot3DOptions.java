package io.coolbox.graphics;
/** Options for a single-series projected 3D chart (Python: {@code graphics.Plot3DOptions}). */
public record Plot3DOptions(int width,int height,String title,String xLabel,String yLabel,String zLabel,String seriesLabel,Color color,Graph3DType type,double azimuthDegrees,double elevationDegrees,boolean showWindow,String savePath) {
 public Plot3DOptions { if(width==0)width=800;if(height==0)height=600;if(xLabel==null||xLabel.isEmpty())xLabel="X";if(yLabel==null||yLabel.isEmpty())yLabel="Y";if(zLabel==null||zLabel.isEmpty())zLabel="Z";if(seriesLabel==null||seriesLabel.isEmpty())seriesLabel="Series";if(color==null)color=Color.BLUE;if(type==null)type=Graph3DType.SCATTER;if(azimuthDegrees==0)azimuthDegrees=45;if(elevationDegrees==0)elevationDegrees=25;if(savePath==null||savePath.isEmpty())savePath="plot.png";if(title==null)title=""; }
 public static Plot3DOptions defaults(){return new Plot3DOptions(800,600,"","X","Y","Z","Series",Color.BLUE,Graph3DType.SCATTER,45,25,false,"plot.png");}
}
