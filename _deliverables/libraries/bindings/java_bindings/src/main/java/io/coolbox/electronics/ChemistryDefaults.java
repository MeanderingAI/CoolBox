package io.coolbox.electronics;
import io.coolbox.internal.ErrorBuffer;
import io.coolbox.internal.Matrices;
import io.coolbox.internal.NativeHandle;
import com.sun.jna.Pointer;
/** Chemistry voltage and capacity defaults equivalent to Python {@code ChemistryDefaults}. */
public final class ChemistryDefaults extends NativeHandle {
 private ChemistryDefaults(Pointer p){super(p,ElectronicsNative.INSTANCE::coolbox_electronics_defaults_free);}
 public static ChemistryDefaults forChemistry(Chemistry chemistry){ErrorBuffer e=new ErrorBuffer();return new ChemistryDefaults(e.check(ElectronicsNative.INSTANCE.coolbox_electronics_defaults(chemistry.ordinal(),e.pointer(),e.capacity())));}
 public double nominalVoltage(){return value(0);} public double maxVoltage(){return value(1);} public double minVoltage(){return value(2);} public double typicalCapacity(){return value(3);} public double typicalInternalResistance(){return value(4);}
 private double value(int i){return ElectronicsNative.INSTANCE.coolbox_electronics_default_value(handle(),i);}
 public static String chemistryName(Chemistry chemistry){return Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_chemistry_name(chemistry.ordinal()));}
}
