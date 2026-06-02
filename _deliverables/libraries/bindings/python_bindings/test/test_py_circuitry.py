import py_circuitry
import json

def test_solve_circuit_json():
    # Example circuit JSON (series battery and resistor)
    circuit_json = json.dumps([
        {
            "type": "battery",
            "x1": 0, "y1": 0, "x2": 0, "y2": 1,
            "label": "V1", "value": "10 V", "resistance": "0.1"
        },
        {
            "type": "resistor",
            "x1": 0, "y1": 1, "x2": 1, "y2": 1,
            "label": "R1", "value": "2"
        },
        {
            "type": "wire",
            "x1": 1, "y1": 1, "x2": 0, "y2": 0,
            "label": "", "value": ""
        }
    ])
    result = py_circuitry.solve_circuit_json(circuit_json)
    print("Node voltages:", result.node_voltages)
    for comp in result.component_results:
        print(f"{comp.label} ({comp.type_name}): I={comp.current}, V={comp.voltage_drop}, P={comp.power}")

if __name__ == "__main__":
    test_solve_circuit_json()
