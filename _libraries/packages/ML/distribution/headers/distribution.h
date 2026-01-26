#ifndef DISTRIBUTION_H
#define DISTRIBUTION_H

#include <string>

/**
 * @class Distribution
 * @brief An abstract base class for a probability distribution.
 *
 * Provides a common interface for probability density/mass function (PDF),
 * cumulative distribution function (CDF), random sampling, and key functions
 * for use in a Generalized Linear Model (GLM).
 */
class Distribution {
public:
    // Pure virtual destructor ensures proper cleanup of derived classes.
    virtual ~Distribution() = default;

    /**
     * @brief Calculates the probability density/mass function (PDF).
     * @param x The value at which to evaluate the PDF.
     * @return The PDF value at x.
     */
    virtual double pdf(double x) const = 0;

    /**
     * @brief Calculates the natural logarithm of the PDF.
     * @param x The value at which to evaluate the log-PDF.
     * @return The log-PDF value at x.
     */
    virtual double log_pdf(double x) const = 0;

    /**
     * @brief Calculates the cumulative distribution function (CDF).
     * @param x The value at which to evaluate the CDF.
     * @return The CDF value at x.
     */
    virtual double cdf(double x) const = 0;

    /**
     * @brief Calculates the natural logarithm of the CDF.
     * @param x The value at which to evaluate the log-CDF.
     * @return The log-CDF value at x.
     */
    virtual double log_cdf(double x) const = 0;
    
    /**
     * @brief Samples a single random value from the distribution.
     * @return A random value.
     */
    virtual double sample() = 0;

    // --- GLM-related functions ---
    
    /**
     * @brief Returns the name of the canonical link function for this distribution.
     * @return A string representing the link function name (e.g., "identity", "log", "logit").
     */
    virtual std::string link_name() const = 0;

    /**
     * @brief The link function, g(mu) = eta.
     * @param mu The mean of the distribution.
     * @return The linear predictor (eta).
     */
    virtual double link_function(double mu) const = 0;

    /**
     * @brief The inverse link function, mu = g^{-1}(eta).
     * @param eta The linear predictor.
     * @return The mean of the distribution (mu).
     */
    virtual double mean_function(double eta) const = 0;
};

#endif // DISTRIBUTION_H