#ifndef DISTRIBUTION_UTIL_H
#define DISTRIBUTION_UTIL_H

#include <stdexcept>
#include <cmath>

/**
 * @brief Computes the factorial of a non-negative integer.
 * @param n The non-negative integer.
 * @return The factorial of n.
 */
long long factorial(int n);

/**
 * @brief Computes the number of combinations "n choose k".
 * @param n The total number of items.
 * @param k The number of items to choose.
 * @return The number of combinations.
 */
long long combinations(int n, int k);

/**
 * @brief The logistic function (sigmoid).
 * @param x The input value.
 * @return The logistic function result.
 */
double logistic_function(double x);


#endif