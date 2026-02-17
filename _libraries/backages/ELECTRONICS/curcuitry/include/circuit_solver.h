#pragma once

#include "circuit.h"
#include "component.h"
#include "battery.h"
#include "resistor.h"
#include "wire.h"

#include <Eigen/Dense>

#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <string>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <iomanip>

namespace curcuitry {

// ===================================================================
// Solution data structures
// ===================================================================

/**
 * @brief Per-component analysis result.
 */
struct ComponentResult {
    std::string label;
    std::string type_name;

    // Resistor fields
    double resistance     = 0.0;
    double voltage_drop   = 0.0;
    double current        = 0.0;
    double power          = 0.0;

    // Battery fields
    double emf                  = 0.0;
    double internal_resistance  = 0.0;
    double terminal_voltage     = 0.0;
};

/**
 * @brief Full solution of a circuit.
 */
struct CircuitSolution {
    /** Node index -> voltage (ground node is 0V). */
    std::map<int, double> node_voltages;

    /** Per-component detailed results. */
    std::vector<ComponentResult> component_results;

    /** Total current supplied by all sources. */
    double total_current = 0.0;
};

// ===================================================================
// Union-Find for merging wire-connected nodes
// ===================================================================

/**
 * @brief Disjoint-set (union-find) keyed on Point.
 */
class UnionFind {
private:
    std::map<Point, Point> parent_;

public:
    Point find(const Point& p) {
        if (parent_.find(p) == parent_.end()) {
            parent_[p] = p;
        }
        if (parent_[p] == p) return p;
        parent_[p] = find(parent_[p]);   // path compression
        return parent_[p];
    }

    void unite(const Point& a, const Point& b) {
        Point pa = find(a);
        Point pb = find(b);
        if (!(pa == pb)) {
            parent_[pa] = pb;
        }
    }
};

// ===================================================================
// CircuitSolver
// ===================================================================

/**
 * @brief Solves a Circuit using Modified Nodal Analysis (MNA).
 *
 * Algorithm overview:
 *   1. Collect all unique coordinate endpoints.
 *   2. Merge endpoints connected by wires (Union-Find).
 *   3. Assign integer node IDs to each merged group.
 *   4. Decompose batteries with internal resistance into an
 *      ideal voltage source + series resistor (intermediate node).
 *   5. Assemble the MNA matrix:
 *        [G  B] [v]   [i]
 *        [C  D] [j] = [e]
 *      G = conductance matrix
 *      B,C = voltage-source incidence
 *      D = 0 (ideal sources)
 *      v = node voltages, j = source currents
 *      i = current-source injections (0 for this solver)
 *      e = known source voltages
 *   6. Solve the linear system with Eigen.
 *   7. Extract per-component voltages, currents, and power.
 */
class CircuitSolver {
private:
    const Circuit& circuit_;

    // Node mapping: Point -> merged node ID
    std::unordered_map<Point, int, PointHash> node_map_;

    int ground_node_ = -1;

    // Internal structures built during solve()
    struct IdealVoltageSource {
        int    n_pos;      // node at higher potential
        int    n_neg;      // node at lower potential
        double voltage;
        std::string label;
    };

    struct InternalResistor {
        int    n1;
        int    n2;
        double resistance;
        std::string label;
    };

public:
    explicit CircuitSolver(const Circuit& circuit) : circuit_(circuit) {}

    /**
     * @brief Solve the circuit and return a CircuitSolution.
     * @throws std::runtime_error if the system is singular.
     */
    CircuitSolution solve();

    /**
     * @brief Pretty-print a solution to stdout.
     */
    static void print_results(const CircuitSolution& sol);

private:
    /**
     * @brief Build the node map by collecting all component endpoints
     *        and merging those connected by wires.
     * @return Total number of unique (merged) nodes.
     */
    int build_nodes();
};

// ===================================================================
// Implementation
// ===================================================================

inline int CircuitSolver::build_nodes() {
    UnionFind uf;

    // Register every endpoint
    for (const auto& comp : circuit_.components()) {
        uf.find(comp->node1());
        uf.find(comp->node2());
    }

    // Merge wire-connected points
    for (const Wire* w : circuit_.wires()) {
        uf.unite(w->node1(), w->node2());
    }

    // Assign contiguous integer IDs to each canonical representative
    std::map<Point, int> canonical;
    int next_id = 0;

    // Collect all points
    std::set<Point> all_points;
    for (const auto& comp : circuit_.components()) {
        all_points.insert(comp->node1());
        all_points.insert(comp->node2());
    }

    for (const Point& p : all_points) {
        Point root = uf.find(p);
        if (canonical.find(root) == canonical.end()) {
            canonical[root] = next_id++;
        }
        node_map_[p] = canonical[root];
    }

    return next_id;
}

inline CircuitSolution CircuitSolver::solve() {
    node_map_.clear();
    int num_nodes = build_nodes();

    // ------------------------------------------------------------------
    // Decompose batteries: ideal voltage source + optional series R_int
    // ------------------------------------------------------------------
    std::vector<IdealVoltageSource> vsources;
    std::vector<InternalResistor>   extra_resistors;
    int next_node_id = num_nodes;

    for (const Battery* bat : circuit_.batteries()) {
        int n_pos = node_map_[bat->node1()];   // positive terminal
        int n_neg = node_map_[bat->node2()];   // negative terminal

        if (bat->internal_resistance() > 0.0) {
            // Create intermediate node between source and + terminal
            int int_node = next_node_id++;
            // Ideal source raises potential from n_neg to int_node
            vsources.push_back({int_node, n_neg, bat->voltage(), bat->label()});
            // Internal resistance from int_node to n_pos
            extra_resistors.push_back({int_node, n_pos,
                                       bat->internal_resistance(),
                                       bat->label() + "_Rint"});
        } else {
            vsources.push_back({n_pos, n_neg, bat->voltage(), bat->label()});
        }
    }

    int total_nodes  = next_node_id;
    int num_vsources = static_cast<int>(vsources.size());

    // ------------------------------------------------------------------
    // Select ground node (negative terminal of first voltage source)
    // ------------------------------------------------------------------
    if (!vsources.empty()) {
        ground_node_ = vsources[0].n_neg;
    } else {
        ground_node_ = 0;
    }

    // ------------------------------------------------------------------
    // Build index mapping (exclude ground from unknowns)
    // ------------------------------------------------------------------
    std::map<int, int> node_to_idx;
    int idx = 0;
    for (int n = 0; n < total_nodes; ++n) {
        if (n != ground_node_) {
            node_to_idx[n] = idx++;
        }
    }

    int n_vars     = total_nodes - 1;   // voltage unknowns
    int matrix_size = n_vars + num_vsources;

    Eigen::MatrixXd A = Eigen::MatrixXd::Zero(matrix_size, matrix_size);
    Eigen::VectorXd z = Eigen::VectorXd::Zero(matrix_size);

    // ------------------------------------------------------------------
    // Stamp resistors into G sub-matrix
    // ------------------------------------------------------------------
    auto stamp_resistor = [&](int n1, int n2, double R) {
        if (R <= 0.0) return;
        double g = 1.0 / R;
        if (n1 != ground_node_) {
            int i = node_to_idx[n1];
            A(i, i) += g;
        }
        if (n2 != ground_node_) {
            int j = node_to_idx[n2];
            A(j, j) += g;
        }
        if (n1 != ground_node_ && n2 != ground_node_) {
            int i = node_to_idx[n1];
            int j = node_to_idx[n2];
            A(i, j) -= g;
            A(j, i) -= g;
        }
    };

    // External resistors from the circuit
    for (const Resistor* r : circuit_.resistors()) {
        int n1 = node_map_[r->node1()];
        int n2 = node_map_[r->node2()];
        stamp_resistor(n1, n2, r->resistance());
    }

    // Internal resistors from battery decomposition
    for (const auto& ir : extra_resistors) {
        stamp_resistor(ir.n1, ir.n2, ir.resistance);
    }

    // ------------------------------------------------------------------
    // Stamp voltage sources into B, C sub-matrices
    // ------------------------------------------------------------------
    for (int k = 0; k < num_vsources; ++k) {
        int vs_col = n_vars + k;   // column in A for this source current

        int np = vsources[k].n_pos;
        int nn = vsources[k].n_neg;

        // B stamp (node equations)
        if (np != ground_node_) {
            int i = node_to_idx[np];
            A(i, vs_col) += 1.0;    // current enters n_pos
        }
        if (nn != ground_node_) {
            int j = node_to_idx[nn];
            A(j, vs_col) -= 1.0;    // current leaves n_neg
        }

        // C stamp (voltage constraint: V(n_pos) - V(n_neg) = E)
        if (np != ground_node_) {
            int i = node_to_idx[np];
            A(vs_col, i) += 1.0;
        }
        if (nn != ground_node_) {
            int j = node_to_idx[nn];
            A(vs_col, j) -= 1.0;
        }

        // RHS: known source voltage
        z(vs_col) = vsources[k].voltage;
    }

    // ------------------------------------------------------------------
    // Solve  A * x = z
    // ------------------------------------------------------------------
    Eigen::VectorXd x = A.fullPivLu().solve(z);

    // Verify the solution
    double residual = (A * x - z).norm();
    if (residual > 1e-6) {
        throw std::runtime_error(
            "Circuit has no unique solution (residual=" +
            std::to_string(residual) +
            "). Check for loops of voltage sources or disconnected nodes.");
    }

    // ------------------------------------------------------------------
    // Extract results
    // ------------------------------------------------------------------
    CircuitSolution sol;

    // Node voltages
    for (int n = 0; n < total_nodes; ++n) {
        if (n == ground_node_) {
            sol.node_voltages[n] = 0.0;
        } else {
            sol.node_voltages[n] = x(node_to_idx[n]);
        }
    }

    // Voltage source currents
    // Convention: x[n_vars + k] is current flowing from n_pos to n_neg
    // through the source. Negative means current flows n_neg -> n_pos
    // (normal discharging direction).
    std::map<std::string, double> vs_currents;
    for (int k = 0; k < num_vsources; ++k) {
        double j_vs = x(n_vars + k);
        vs_currents[vsources[k].label] = j_vs;
    }

    // Resistor analysis
    for (const Resistor* r : circuit_.resistors()) {
        int n1 = node_map_[r->node1()];
        int n2 = node_map_[r->node2()];
        double v1 = sol.node_voltages[n1];
        double v2 = sol.node_voltages[n2];
        double v_drop = v1 - v2;
        double current = (r->resistance() > 0.0) ? v_drop / r->resistance() : 0.0;
        double power = std::abs(v_drop * current);

        ComponentResult cr;
        cr.label        = r->label().empty()
                            ? ("R(" + std::to_string((int)r->node1().x) + ","
                               + std::to_string((int)r->node1().y) + ")")
                            : r->label();
        cr.type_name    = "Resistor";
        cr.resistance   = r->resistance();
        cr.voltage_drop = v_drop;
        cr.current      = current;
        cr.power        = power;
        sol.component_results.push_back(cr);
    }

    // Battery analysis
    for (const Battery* bat : circuit_.batteries()) {
        int n_pos = node_map_[bat->node1()];
        int n_neg = node_map_[bat->node2()];
        double v_pos = sol.node_voltages[n_pos];
        double v_neg = sol.node_voltages[n_neg];

        // Current through battery: the MNA variable is for the ideal
        // source inside. Negate because MNA convention defines current
        // from + to − through source; physical current is opposite.
        double i_source = 0.0;
        auto it = vs_currents.find(bat->label());
        if (it != vs_currents.end()) {
            i_source = -(it->second);   // positive = discharging
        }

        ComponentResult cr;
        cr.label               = bat->label();
        cr.type_name           = "Battery";
        cr.emf                 = bat->voltage();
        cr.internal_resistance = bat->internal_resistance();
        cr.terminal_voltage    = v_pos - v_neg;
        cr.current             = i_source;
        cr.power               = std::abs(cr.terminal_voltage * i_source);
        sol.component_results.push_back(cr);

        sol.total_current += i_source;
    }

    return sol;
}

inline void CircuitSolver::print_results(const CircuitSolution& sol) {
    std::cout << std::fixed << std::setprecision(4);

    std::cout << "\n╔══════════════════════════════════════════╗\n";
    std::cout <<   "║         CIRCUIT SOLUTION                 ║\n";
    std::cout <<   "╚══════════════════════════════════════════╝\n\n";

    // Node voltages
    std::cout << "── Node Voltages ──────────────────────────\n";
    for (const auto& [node, voltage] : sol.node_voltages) {
        std::cout << "  Node " << node << ":  "
                  << std::setw(10) << voltage << " V";
        if (std::abs(voltage) < 1e-12) std::cout << "  (ground)";
        std::cout << "\n";
    }

    // Component details
    std::cout << "\n── Component Analysis ─────────────────────\n";
    for (const auto& cr : sol.component_results) {
        std::cout << "\n  " << cr.type_name << " [" << cr.label << "]\n";
        if (cr.type_name == "Resistor") {
            std::cout << "    Resistance:     " << std::setw(10) << cr.resistance   << " Ω\n";
            std::cout << "    Voltage drop:   " << std::setw(10) << cr.voltage_drop << " V\n";
            std::cout << "    Current:        " << std::setw(10) << cr.current      << " A\n";
            std::cout << "    Power dissip.:  " << std::setw(10) << cr.power        << " W\n";
        } else if (cr.type_name == "Battery") {
            std::cout << "    EMF:            " << std::setw(10) << cr.emf                 << " V\n";
            std::cout << "    Int. resistance:" << std::setw(10) << cr.internal_resistance << " Ω\n";
            std::cout << "    Terminal V:     " << std::setw(10) << cr.terminal_voltage    << " V\n";
            std::cout << "    Current:        " << std::setw(10) << cr.current             << " A\n";
            std::cout << "    Power delivered:" << std::setw(10) << cr.power               << " W\n";
        }
    }

    std::cout << "\n── Summary ────────────────────────────────\n";
    std::cout << "  Total source current: "
              << std::setw(10) << sol.total_current << " A\n";
    std::cout << "\n";
}

} // namespace curcuitry
