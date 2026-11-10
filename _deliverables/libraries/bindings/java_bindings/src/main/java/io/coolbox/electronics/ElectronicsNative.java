package io.coolbox.electronics;
import com.sun.jna.Library;
import com.sun.jna.Pointer;
import io.coolbox.internal.NativeLibraries;
interface ElectronicsNative extends Library {
 ElectronicsNative INSTANCE=NativeLibraries.bridge(ElectronicsNative.class);
 Pointer coolbox_electronics_chemistry_name(int chemistry);
 Pointer coolbox_electronics_defaults(int chemistry,Pointer error,long cap);void coolbox_electronics_defaults_free(Pointer p);double coolbox_electronics_default_value(Pointer p,int i);
 Pointer coolbox_electronics_cell_new(String label,int chemistry,double capacity,double resistance,double soc,int custom,Pointer error,long cap);Pointer coolbox_electronics_cell_copy(Pointer p);void coolbox_electronics_cell_free(Pointer p);double coolbox_electronics_cell_value(Pointer p,int i);int coolbox_electronics_cell_flag(Pointer p,int i);int coolbox_electronics_cell_chemistry(Pointer p);Pointer coolbox_electronics_cell_label(Pointer p);Pointer coolbox_electronics_cell_string(Pointer p);double coolbox_electronics_cell_terminal(Pointer p,double current);double coolbox_electronics_cell_transfer(Pointer p,double current,double seconds,int charge);void coolbox_electronics_cell_set(Pointer p,int which,double value);
 Pointer coolbox_electronics_pack_new(String label,int series,int parallel,Pointer cell,Pointer error,long cap);void coolbox_electronics_pack_free(Pointer p);double coolbox_electronics_pack_value(Pointer p,int i);double coolbox_electronics_pack_terminal(Pointer p,double current);int coolbox_electronics_pack_depleted(Pointer p);double coolbox_electronics_pack_transfer(Pointer p,double current,double seconds,int charge);Pointer coolbox_electronics_pack_cell(Pointer p,int series,int parallel,Pointer error,long cap);Pointer coolbox_electronics_pack_label(Pointer p);Pointer coolbox_electronics_pack_string(Pointer p);
 Pointer coolbox_electronics_solve_circuit(String json,Pointer error,long cap);void coolbox_electronics_solution_free(Pointer p);long coolbox_electronics_node_count(Pointer p);int coolbox_electronics_node_index(Pointer p,long i);double coolbox_electronics_node_voltage(Pointer p,long i);long coolbox_electronics_component_count(Pointer p);Pointer coolbox_electronics_component_string(Pointer p,long i,int which);double coolbox_electronics_component_value(Pointer p,long i,int which);
}
