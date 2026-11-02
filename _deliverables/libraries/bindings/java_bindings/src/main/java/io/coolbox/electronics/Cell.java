package io.coolbox.electronics;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
/** Mutable battery cell equivalent to Python {@code battery_lib.Cell}. */
public final class Cell extends NativeHandle {
 public Cell(String label,Chemistry chemistry,double capacityAh,double internalResistance,double initialSoc){this(create(label,chemistry,capacityAh,internalResistance,initialSoc,true));}
 public Cell(String label,Chemistry chemistry,double capacityAh,double internalResistance){this(label,chemistry,capacityAh,internalResistance,1.0);}
 public Cell(String label,Chemistry chemistry,double initialSoc){this(create(label,chemistry,0,0,initialSoc,false));}
 public Cell(String label,Chemistry chemistry){this(label,chemistry,1.0);}
 Cell(Pointer p){super(p,ElectronicsNative.INSTANCE::coolbox_electronics_cell_free);}
 static Cell fromOwned(Pointer p){return new Cell(p);}
 private static Pointer create(String label,Chemistry chemistry,double capacity,double resistance,double soc,boolean custom){ErrorBuffer e=new ErrorBuffer();return e.check(ElectronicsNative.INSTANCE.coolbox_electronics_cell_new(label,chemistry.ordinal(),capacity,resistance,soc,custom?1:0,e.pointer(),e.capacity()));}
 public double soc(){return value(0);} public double socPercent(){return value(1);} public boolean isDepleted(){return flag(0);} public boolean isFullyCharged(){return flag(1);} public double openCircuitVoltage(){return value(2);} public double maxCurrent(){return value(3);} public double nominalVoltage(){return value(4);} public double maxVoltage(){return value(5);} public double minVoltage(){return value(6);} public double capacityAh(){return value(7);} public double remainingAh(){return value(8);} public double internalResistance(){return value(9);} public double temperatureC(){return value(10);} public int cycleCount(){return (int)value(11);}
 public double terminalVoltage(double currentA){return ElectronicsNative.INSTANCE.coolbox_electronics_cell_terminal(handle(),currentA);} public double discharge(double currentA,double seconds){return transfer(currentA,seconds,false);} public double charge(double currentA,double seconds){return transfer(currentA,seconds,true);} private double transfer(double current,double seconds,boolean charge){return ElectronicsNative.INSTANCE.coolbox_electronics_cell_transfer(handle(),current,seconds,charge?1:0);}
 public void setTemperature(double celsius){ElectronicsNative.INSTANCE.coolbox_electronics_cell_set(handle(),0,celsius);} public void setInternalResistance(double ohms){ElectronicsNative.INSTANCE.coolbox_electronics_cell_set(handle(),1,ohms);}
 public String label(){return Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_cell_label(handle()));} public Chemistry chemistry(){return Chemistry.values()[ElectronicsNative.INSTANCE.coolbox_electronics_cell_chemistry(handle())];} public String toNativeString(){return Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_cell_string(handle()));}
 private double value(int i){return ElectronicsNative.INSTANCE.coolbox_electronics_cell_value(handle(),i);} private boolean flag(int i){return ElectronicsNative.INSTANCE.coolbox_electronics_cell_flag(handle(),i)!=0;}
}


