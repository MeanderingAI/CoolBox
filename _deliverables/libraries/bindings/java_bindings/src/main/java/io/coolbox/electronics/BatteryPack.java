package io.coolbox.electronics;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
/** Battery pack equivalent to Python {@code battery_lib.BatteryPack}. */
public final class BatteryPack extends NativeHandle {
 public BatteryPack(String label,int seriesCount,int parallelCount,Cell prototype){super(create(label,seriesCount,parallelCount,prototype),ElectronicsNative.INSTANCE::coolbox_electronics_pack_free);}
 private static Pointer create(String l,int s,int p,Cell c){ErrorBuffer e=new ErrorBuffer();return e.check(ElectronicsNative.INSTANCE.coolbox_electronics_pack_new(l,s,p,c.handle(),e.pointer(),e.capacity()));}
 public double packVoltage(){return value(0);} public double packCapacityAh(){return value(1);} public double packEnergyWh(){return value(2);} public double packInternalResistance(){return value(3);} public double averageSoc(){return value(4);} public double minSoc(){return value(5);} public int totalCells(){return (int)value(6);} public int seriesCount(){return (int)value(7);} public int parallelCount(){return (int)value(8);}
 public double packTerminalVoltage(double currentA){return ElectronicsNative.INSTANCE.coolbox_electronics_pack_terminal(handle(),currentA);} public boolean isDepleted(){return ElectronicsNative.INSTANCE.coolbox_electronics_pack_depleted(handle())!=0;} public double discharge(double currentA,double seconds){return transfer(currentA,seconds,false);} public double charge(double currentA,double seconds){return transfer(currentA,seconds,true);} private double transfer(double a,double s,boolean c){return ElectronicsNative.INSTANCE.coolbox_electronics_pack_transfer(handle(),a,s,c?1:0);}
 public Cell cell(int series,int parallel){ErrorBuffer e=new ErrorBuffer();return new CellAccess(e.check(ElectronicsNative.INSTANCE.coolbox_electronics_pack_cell(handle(),series,parallel,e.pointer(),e.capacity()))).value();}
 public void printStatus(){System.out.println(toNativeString());}
 public String label(){return Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_pack_label(handle()));} public String toNativeString(){return Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_pack_string(handle()));}
 private double value(int i){return ElectronicsNative.INSTANCE.coolbox_electronics_pack_value(handle(),i);}
 private static final class CellAccess { private final Pointer p; CellAccess(Pointer p){this.p=p;} Cell value(){return Cell.fromOwned(p);} }
}
