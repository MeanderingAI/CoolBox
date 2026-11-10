package io.coolbox.electronics;
import com.sun.jna.Pointer;
import io.coolbox.internal.ErrorBuffer;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
/** Native electronics operations equivalent to Python {@code py_circuitry.solve_circuit_json}. */
public final class Electronics {
 private Electronics() {}
 public static CircuitSolution solveCircuitJson(String json){
  ErrorBuffer error=new ErrorBuffer();Pointer p=error.check(ElectronicsNative.INSTANCE.coolbox_electronics_solve_circuit(json,error.pointer(),error.capacity()));
  try{
   Map<Integer,Double> nodes=new LinkedHashMap<>();long nc=ElectronicsNative.INSTANCE.coolbox_electronics_node_count(p);
   for(long i=0;i<nc;i++)nodes.put(ElectronicsNative.INSTANCE.coolbox_electronics_node_index(p,i),ElectronicsNative.INSTANCE.coolbox_electronics_node_voltage(p,i));
   List<ComponentResult> results=new ArrayList<>();long count=ElectronicsNative.INSTANCE.coolbox_electronics_component_count(p);
   for(long i=0;i<count;i++){
    String label=io.coolbox.internal.Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_component_string(p,i,0));
    String type=io.coolbox.internal.Matrices.takeString(ElectronicsNative.INSTANCE.coolbox_electronics_component_string(p,i,1));
    double[] v=new double[7];for(int j=0;j<v.length;j++)v[j]=ElectronicsNative.INSTANCE.coolbox_electronics_component_value(p,i,j);
    results.add(new ComponentResult(label,type,v[0],v[1],v[2],v[3],v[4],v[5],v[6]));
   }
   return new CircuitSolution(Map.copyOf(nodes),List.copyOf(results));
  } finally {ElectronicsNative.INSTANCE.coolbox_electronics_solution_free(p);}
 }
}
