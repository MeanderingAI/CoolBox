package io.coolbox.electronics;
import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;
class ElectronicsBindingsTest {
 @Test void modelsCellsPacksAndCircuitSolutions(){
  var defaults=ChemistryDefaults.forChemistry(Chemistry.LITHIUM_ION);
  try(defaults){assertEquals(3.7,defaults.nominalVoltage(),1e-9);assertEquals("Li-ion",ChemistryDefaults.chemistryName(Chemistry.LITHIUM_ION));}
  var cell=new Cell("test",Chemistry.LITHIUM_ION,2.5,0.05,0.8);
  try(cell){assertEquals(0.8,cell.soc(),1e-9);assertTrue(cell.terminalVoltage(1)>0);var pack=new BatteryPack("pack",2,1,cell);try(pack){assertEquals(2,pack.totalCells());assertTrue(pack.packVoltage()>cell.nominalVoltage());assertEquals(0.8,pack.averageSoc(),1e-9);try(var copy=pack.cell(0,0)){assertEquals("pack_s0p0",copy.label());}}}
  var solved=Electronics.solveCircuitJson("[{\"type\":\"battery\",\"x1\":0,\"y1\":0,\"x2\":1,\"y2\":0,\"label\":\"V1\",\"value\":\"9 V\",\"resistance\":\"1\"},{\"type\":\"resistor\",\"x1\":1,\"y1\":0,\"x2\":1,\"y2\":1,\"label\":\"R1\",\"value\":\"8 Ohm\"},{\"type\":\"wire\",\"x1\":1,\"y1\":1,\"x2\":0,\"y2\":0,\"label\":\"W1\",\"value\":\"\"}]");
  assertEquals(3,solved.componentResults().size());assertEquals(1.0,Math.abs(solved.componentResults().get(1).current()),1e-9);
  assertThrows(RuntimeException.class,()->Electronics.solveCircuitJson("not json"));
 }
}

