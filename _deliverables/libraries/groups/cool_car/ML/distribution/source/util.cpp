#include "util.h"

long long factorial(int n) {
    if (n < 0) throw std::invalid_argument("Factorial is not defined for negative numbers.");
    long long result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

long long combinations(int n, int k) {
    if (k < 0 || k > n) throw std::invalid_argument("Invalid arguments for combinations.");
    if (k == 0 || k == n) return 1;
    // Use the smaller of k and n-k for efficiency
    if (k > n - k) k = n - k;
    long long result = 1;
    for (int i = 0; i < k; ++i) {
        result = result * (n - i) / (i + 1);
    }
    return result;
}

double logistic_function(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}
