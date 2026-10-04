#include "../headers/quantum_simulator.h"

#include <sstream>
#include <stdexcept>

namespace quantum {

QuantumCircuit::QuantumCircuit(int num_qubits) : num_qubits_(num_qubits) {
    if (num_qubits <= 0) throw std::invalid_argument("QuantumCircuit: num_qubits must be positive");
}

void QuantumCircuit::require_qubit(int qubit) const {
    if (qubit < 0 || qubit >= num_qubits_) {
        throw std::out_of_range("QuantumCircuit: qubit index " + std::to_string(qubit) +
                                " out of range for a " + std::to_string(num_qubits_) + "-qubit circuit");
    }
}

QuantumCircuit& QuantumCircuit::h(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"H", {}, qubit, gates::hadamard(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::x(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"X", {}, qubit, gates::pauli_x(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::y(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"Y", {}, qubit, gates::pauli_y(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::z(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"Z", {}, qubit, gates::pauli_z(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::s(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"S", {}, qubit, gates::phase_s(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::t(int qubit) {
    require_qubit(qubit);
    ops_.push_back({"T", {}, qubit, gates::phase_t(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::rx(int qubit, double theta) {
    require_qubit(qubit);
    ops_.push_back({"RX", {}, qubit, gates::rotation_x(theta), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::ry(int qubit, double theta) {
    require_qubit(qubit);
    ops_.push_back({"RY", {}, qubit, gates::rotation_y(theta), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::rz(int qubit, double theta) {
    require_qubit(qubit);
    ops_.push_back({"RZ", {}, qubit, gates::rotation_z(theta), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::phase(int qubit, double theta) {
    require_qubit(qubit);
    ops_.push_back({"PHASE", {}, qubit, gates::phase(theta), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::cnot(int control, int target) {
    require_qubit(control);
    require_qubit(target);
    ops_.push_back({"CNOT", {control}, target, gates::pauli_x(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::cz(int control, int target) {
    require_qubit(control);
    require_qubit(target);
    ops_.push_back({"CZ", {control}, target, gates::pauli_z(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::controlled(const std::vector<int>& controls, int target, const Matrix2& gate, const std::string& label) {
    require_qubit(target);
    for (int c : controls) require_qubit(c);
    ops_.push_back({label, controls, target, gate, false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::toffoli(int control_a, int control_b, int target) {
    require_qubit(control_a);
    require_qubit(control_b);
    require_qubit(target);
    ops_.push_back({"TOFFOLI", {control_a, control_b}, target, gates::pauli_x(), false, -1});
    return *this;
}

QuantumCircuit& QuantumCircuit::swap(int qubit_a, int qubit_b) {
    require_qubit(qubit_a);
    require_qubit(qubit_b);
    ops_.push_back({"SWAP", {}, qubit_a, Matrix2{}, true, qubit_b});
    return *this;
}

void QuantumCircuit::apply(QuantumState& state) const {
    if (state.num_qubits() != num_qubits_) {
        throw std::invalid_argument("QuantumCircuit::apply: state has " + std::to_string(state.num_qubits()) +
                                    " qubits, circuit expects " + std::to_string(num_qubits_));
    }
    for (const auto& op : ops_) {
        if (op.is_swap) {
            state.apply_swap(op.target, op.swap_other);
        } else {
            state.apply_multi_controlled_gate(op.controls, op.target, op.gate);
        }
    }
}

std::string QuantumCircuit::describe() const {
    std::ostringstream out;
    for (const auto& op : ops_) {
        if (op.is_swap) {
            out << "SWAP q" << op.target << " <-> q" << op.swap_other << '\n';
            continue;
        }
        out << op.label;
        if (!op.controls.empty()) {
            out << " q";
            for (std::size_t i = 0; i < op.controls.size(); ++i) {
                if (i > 0) out << ",q";
                out << op.controls[i];
            }
            out << " ->";
        }
        out << " q" << op.target << '\n';
    }
    return out.str();
}

} // namespace quantum
