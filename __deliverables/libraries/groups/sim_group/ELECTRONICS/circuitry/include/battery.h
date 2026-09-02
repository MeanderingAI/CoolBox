#ifndef CIRCUITRY_BATTERY_H
#define CIRCUITRY_BATTERY_H

#include "component.h"
#include <sstream>

namespace circuitry {

/**
 * @brief A battery / voltage source component.
 *
 * Models an ideal voltage source with optional internal resistance.
 * - node1 (x1, y1) is the POSITIVE terminal.
 * - node2 (x2, y2) is the NEGATIVE terminal.
 * - voltage is the EMF in volts.
 * - internal_resistance is the series resistance in ohms (default 0).
 *
 * JSON fields:
 *   "type": "battery"
 *   "value": "10 V"
 *   "resistance": "0.1 a9"   (optional, default 0)
 */
class Battery : public Component {
private:
    double voltage_;
    double internal_resistance_;

public:
    Battery()
        : Component(), voltage_(0.0), internal_resistance_(0.0) {}

    Battery(double x1, double y1, double x2, double y2,
            const std::string& label, const std::string& value,
            double voltage, double internal_resistance = 0.0)
        : Component(x1, y1, x2, y2, label, value),
          voltage_(voltage),
          internal_resistance_(internal_resistance) {}

    // ...existing code...

};

} // namespace circuitry

#endif // CIRCUITRY_BATTERY_H