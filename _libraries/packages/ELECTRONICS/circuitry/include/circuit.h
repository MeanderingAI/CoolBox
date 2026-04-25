#ifndef COOLBOX__LIBRARIES_PACKAGES_ELECTRONICS_CIRCUITRY_INCLUDE_CIRCUIT_H
#define COOLBOX__LIBRARIES_PACKAGES_ELECTRONICS_CIRCUITRY_INCLUDE_CIRCUIT_H

#include "component.h"
#include "battery.h"
#include "resistor.h"
#include "wire.h"

#include <vector>
#include <memory>
#include <string>
#include <sstream>
#include <stdexcept>

namespace circuitry {

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
 *     "label": "Vth", "value": "10 V", "resistance": "0.1 a9"
 *   },
 *   {
 *     "type": "resistor",
 *     "x1": 300, "y1": 300, "x2": 500, "y2": 300,
 *     "label": "Rth", "value": "2 a9"
 *   },
 *   {
 *     "type": "wire",
 *     "x1": 200, "y1": 300, "x2": 300, "y2": 300,
 *     "label": "", "value": ""