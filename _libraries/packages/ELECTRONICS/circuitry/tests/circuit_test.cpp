/**
 * @file circuit_test.cpp
 * @brief Test suite for the circuitry library.
 *
 * Tests cover:
 *   - Component construction and field parsing
 *   - Circuit JSON parsing
 *   - MNA solver correctness for series, parallel, and mixed circuits
 */

#include "tyst_framework.hpp"
#include "circuitry.h"

#include <cmath>
#include <string>

using namespace circuitry;

static constexpr double TOL = 1e-4;

// ...existing code...
