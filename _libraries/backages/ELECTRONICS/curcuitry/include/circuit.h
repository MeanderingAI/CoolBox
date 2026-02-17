#pragma once

#include "component.h"
#include "battery.h"
#include "resistor.h"
#include "wire.h"

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <stdexcept>

namespace curcuitry {

/**
 * @brief Container and builder for a circuit composed of Components.
 *
 * Parses a JSON array of component descriptors and stores typed
 * component objects. Provides accessors to iterate over all
 * components or filter by type.
 *
 * Expected JSON format (array of objects):
 * @code
 * [
 *   {
 *     "type": "battery",
 *     "x1": 200, "y1": 200, "x2": 200, "y2": 300,
 *     "label": "Vth", "value": "10 V", "resistance": "0.1 Ω"
 *   },
 *   {
 *     "type": "resistor",
 *     "x1": 300, "y1": 300, "x2": 500, "y2": 300,
 *     "label": "Rth", "value": "2 Ω"
 *   },
 *   {
 *     "type": "wire",
 *     "x1": 200, "y1": 300, "x2": 300, "y2": 300,
 *     "label": "", "value": ""
 *   }
 * ]
 * @endcode
 */
class Circuit {
private:
    std::vector<std::shared_ptr<Component>> components_;
    std::vector<Battery*>  batteries_;
    std::vector<Resistor*> resistors_;
    std::vector<Wire*>     wires_;

public:
    Circuit() = default;

    // ---------------------------------------------------------------
    // Builder methods
    // ---------------------------------------------------------------

    /** @brief Add a pre-constructed battery. */
    void add_battery(const Battery& bat) {
        auto ptr = std::make_shared<Battery>(bat);
        batteries_.push_back(ptr.get());
        components_.push_back(ptr);
    }

    /** @brief Add a pre-constructed resistor. */
    void add_resistor(const Resistor& res) {
        auto ptr = std::make_shared<Resistor>(res);
        resistors_.push_back(ptr.get());
        components_.push_back(ptr);
    }

    /** @brief Add a pre-constructed wire. */
    void add_wire(const Wire& w) {
        auto ptr = std::make_shared<Wire>(w);
        wires_.push_back(ptr.get());
        components_.push_back(ptr);
    }

    // ---------------------------------------------------------------
    // JSON parsing  (minimal hand-rolled parser, no external dep)
    // ---------------------------------------------------------------

    /**
     * @brief Build a Circuit from a JSON string.
     *
     * Performs lightweight parsing of the JSON array format shown
     * in the class documentation. Supports the three component
     * types: battery, resistor, wire.
     *
     * @throws std::runtime_error on malformed JSON or unknown type.
     */
    static Circuit from_json(const std::string& json);

    // ---------------------------------------------------------------
    // Accessors
    // ---------------------------------------------------------------

    const std::vector<std::shared_ptr<Component>>& components() const { return components_; }
    const std::vector<Battery*>&  batteries()  const { return batteries_; }
    const std::vector<Resistor*>& resistors()  const { return resistors_; }
    const std::vector<Wire*>&     wires()      const { return wires_; }

    size_t size() const { return components_.size(); }

    /** @brief Pretty-print circuit component listing. */
    std::string to_string() const {
        std::ostringstream oss;
        oss << "Circuit (" << components_.size() << " components):\n";
        for (const auto& c : components_) {
            oss << "  " << c->to_string() << "\n";
        }
        return oss.str();
    }
};

} // namespace curcuitry
