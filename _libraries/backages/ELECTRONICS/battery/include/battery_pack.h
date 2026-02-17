#pragma once

#include "cell.h"

#include <vector>
#include <string>
#include <sstream>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <iostream>

namespace battery {

/**
 * @brief Models a battery pack composed of cells in a series–parallel
 *        (SxP) configuration.
 *
 * - `series_count` (S): number of cells in series.  Voltages add.
 * - `parallel_count` (P): parallel strings.  Capacities add,
 *   internal resistance divides.
 *
 * Total cells = S × P.
 *
 * Pack voltage   = sum of per-series-stage open-circuit voltages.
 * Pack capacity  = single-cell capacity × P.
 * Pack R_internal = (sum of per-series-stage R) / P.
 *
 * The pack stores individual Cell objects so per-cell SoC imbalance
 * can be modelled.  For a balanced pack, all cells share the same
 * state, but the structure supports cell-level queries.
 *
 * Cell indexing: cell(s, p) where s ∈ [0, S), p ∈ [0, P).
 */
class BatteryPack {
private:
    std::string label_;
    int series_count_;
    int parallel_count_;
    std::vector<Cell> cells_;   // stored row-major: index = s * P + p

    int idx(int s, int p) const { return s * parallel_count_ + p; }

public:
    /**
     * @brief Construct a uniform battery pack.
     *
     * All cells are identical clones of `prototype_cell`, with labels
     * auto-generated as "label_s0p0", "label_s0p1", etc.
     */
    BatteryPack(const std::string& label,
                int series_count,
                int parallel_count,
                const Cell& prototype_cell)
        : label_(label),
          series_count_(series_count),
          parallel_count_(parallel_count)
    {
        if (series_count < 1 || parallel_count < 1)
            throw std::invalid_argument("Series and parallel counts must be >= 1");

        cells_.reserve(series_count * parallel_count);
        for (int s = 0; s < series_count; ++s) {
            for (int p = 0; p < parallel_count; ++p) {
                std::string cell_label = label + "_s"
                    + std::to_string(s) + "p" + std::to_string(p);
                Cell c(cell_label,
                       prototype_cell.chemistry(),
                       prototype_cell.capacity_ah(),
                       prototype_cell.internal_resistance(),
                       prototype_cell.soc());
                cells_.push_back(c);
            }
        }
    }

    // ---------------------------------------------------------------
    // Pack-level metrics
    // ---------------------------------------------------------------

    /** @brief Pack open-circuit voltage (sum of series stage averages). */
    double pack_voltage() const {
        double v = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            // Average OCV across parallel cells at this stage
            double stage_v = 0.0;
            for (int p = 0; p < parallel_count_; ++p) {
                stage_v += cells_[idx(s, p)].open_circuit_voltage();
            }
            v += stage_v / parallel_count_;
        }
        return v;
    }

    /** @brief Pack terminal voltage under load. */
    double pack_terminal_voltage(double pack_current_a) const {
        // Current splits equally among parallel strings
        double string_current = pack_current_a / parallel_count_;
        double v = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            double stage_v = 0.0;
            for (int p = 0; p < parallel_count_; ++p) {
                stage_v += cells_[idx(s, p)].terminal_voltage(string_current);
            }
            v += stage_v / parallel_count_;
        }
        return v;
    }

    /** @brief Total pack capacity in Ah (parallel cells add). */
    double pack_capacity_ah() const {
        // Use stage 0 as representative
        double cap = 0.0;
        for (int p = 0; p < parallel_count_; ++p) {
            cap += cells_[idx(0, p)].capacity_ah();
        }
        return cap;
    }

    /** @brief Pack energy capacity in Wh. */
    double pack_energy_wh() const {
        double nominal_v = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            nominal_v += cells_[idx(s, 0)].nominal_voltage();
        }
        return pack_capacity_ah() * nominal_v;
    }

    /** @brief Total internal resistance of the pack. */
    double pack_internal_resistance() const {
        // Series: R adds per stage
        // Parallel: 1/R_total = sum(1/R_i)
        double r_total = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            double g_stage = 0.0;  // conductance sum for parallel
            for (int p = 0; p < parallel_count_; ++p) {
                double r = cells_[idx(s, p)].internal_resistance();
                if (r > 0) g_stage += 1.0 / r;
            }
            r_total += (g_stage > 0) ? (1.0 / g_stage) : 0.0;
        }
        return r_total;
    }

    /** @brief Average SoC across all cells. */
    double average_soc() const {
        double total = 0.0;
        for (const auto& c : cells_) total += c.soc();
        return total / cells_.size();
    }

    /** @brief Minimum SoC of any cell (weakest link). */
    double min_soc() const {
        double m = 1.0;
        for (const auto& c : cells_) m = std::min(m, c.soc());
        return m;
    }

    /** @brief Is any cell depleted? */
    bool is_depleted() const {
        for (const auto& c : cells_)
            if (c.is_depleted()) return true;
        return false;
    }

    // ---------------------------------------------------------------
    // Discharge / Charge (uniform distribution)
    // ---------------------------------------------------------------

    /**
     * @brief Discharge the pack at a given current for a duration.
     * Current is split equally among parallel strings.
     * @param current_a  Total pack discharge current.
     * @param seconds    Duration in seconds.
     * @return Total energy delivered in Wh.
     */
    double discharge(double current_a, double seconds) {
        double string_current = current_a / parallel_count_;
        double total_energy = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            for (int p = 0; p < parallel_count_; ++p) {
                total_energy += cells_[idx(s, p)].discharge(
                    string_current, seconds);
            }
        }
        return total_energy;
    }

    /**
     * @brief Charge the pack at a given current for a duration.
     * @param current_a  Total pack charge current.
     * @param seconds    Duration in seconds.
     * @return Total energy absorbed in Wh.
     */
    double charge(double current_a, double seconds) {
        double string_current = current_a / parallel_count_;
        double total_energy = 0.0;
        for (int s = 0; s < series_count_; ++s) {
            for (int p = 0; p < parallel_count_; ++p) {
                total_energy += cells_[idx(s, p)].charge(
                    string_current, seconds);
            }
        }
        return total_energy;
    }

    // ---------------------------------------------------------------
    // Cell access
    // ---------------------------------------------------------------

    Cell& cell(int s, int p) { return cells_[idx(s, p)]; }
    const Cell& cell(int s, int p) const { return cells_[idx(s, p)]; }

    const std::vector<Cell>& cells() const { return cells_; }

    int series_count() const { return series_count_; }
    int parallel_count() const { return parallel_count_; }
    int total_cells() const { return series_count_ * parallel_count_; }
    const std::string& label() const { return label_; }

    // ---------------------------------------------------------------
    // Display
    // ---------------------------------------------------------------

    std::string to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "BatteryPack [" << label_ << "] "
            << series_count_ << "S" << parallel_count_ << "P"
            << " (" << total_cells() << " cells)\n"
            << "  Chemistry:  " << chemistry_name(cells_[0].chemistry()) << "\n"
            << "  Pack V_oc:  " << pack_voltage() << " V\n"
            << "  Capacity:   " << pack_capacity_ah() << " Ah\n"
            << "  Energy:     " << pack_energy_wh() << " Wh\n"
            << "  R_internal: " << pack_internal_resistance() << " Ω\n"
            << "  Avg SoC:    " << (average_soc() * 100.0) << "%\n"
            << "  Min SoC:    " << (min_soc() * 100.0) << "%";
        return oss.str();
    }

    void print_status() const {
        std::cout << "\n╔══════════════════════════════════════════╗\n";
        std::cout <<   "║         BATTERY PACK STATUS              ║\n";
        std::cout <<   "╚══════════════════════════════════════════╝\n\n";
        std::cout << to_string() << "\n\n";

        std::cout << "── Cell Details ───────────────────────────\n";
        for (int s = 0; s < series_count_; ++s) {
            for (int p = 0; p < parallel_count_; ++p) {
                std::cout << "  " << cells_[idx(s, p)].to_string() << "\n";
            }
        }
        std::cout << "\n";
    }
};

} // namespace battery
