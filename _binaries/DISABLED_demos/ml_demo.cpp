#include <iostream>
#include <vector>
#include <iomanip>
#include "ml/distribution/normal_distribution.h"
#include "ml/distribution/binomial_distribution.h"

void demo_normal_distribution() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Normal Distribution Demo            ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    NormalDistribution normal(100.0, 15.0); // mean=100, std_dev=15
    
    std::cout << "Normal distribution with μ=100, σ=15\n\n";
    
    // Test various values
    std::vector<double> test_values = {70, 85, 100, 115, 130};
    
    std::cout << "Value  |  PDF      |  CDF\n";
    std::cout << "-------+-----------+-----------\n";
    
    for (double x : test_values) {
        double pdf = normal.pdf(x);
        double cdf = normal.cdf(x);
        std::cout << std::setw(5) << x << "  |  " 
                  << std::fixed << std::setprecision(6) << pdf << "  |  "
                  << std::setprecision(4) << cdf << "\n";
    }
    
    // Sample generation
    std::cout << "\nGenerating 10 random samples:\n";
    for (int i = 0; i < 10; ++i) {
        double sample = normal.sample();
        std::cout << std::fixed << std::setprecision(2) << sample << " ";
    }
    std::cout << "\n";
}

void demo_binomial_distribution() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Binomial Distribution Demo          ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    BinomialDistribution binomial(10, 0.5); // n=10 trials, p=0.5
    
    std::cout << "Binomial distribution with n=10, p=0.5 (coin flips)\n\n";
    
    std::cout << "k  |  P(X=k)   |  P(X<=k)\n";
    std::cout << "---+-----------+-----------\n";
    
    for (int k = 0; k <= 10; ++k) {
        double pdf_val = binomial.pdf(k);
        double cdf = binomial.cdf(k);
        std::cout << std::setw(2) << k << " |  "
                  << std::fixed << std::setprecision(6) << pdf_val << "  |  "
                  << std::setprecision(6) << cdf << "\n";
    }
    
    // Sample generation
    std::cout << "\nGenerating 20 random samples:\n";
    for (int i = 0; i < 20; ++i) {
        int sample = binomial.sample();
        std::cout << sample << " ";
    }
    std::cout << "\n";
}

void demo_statistical_properties() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Statistical Properties              ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    // Normal distribution statistics
    NormalDistribution normal(50.0, 10.0);
    
    std::cout << "Normal Distribution (μ=50, σ=10):\n";
    std::cout << "  Mean: 50.0\n";
    std::cout << "  Variance: 100.0\n";
    std::cout << "  Std Dev: 10.0\n";
    
    // Binomial distribution statistics
    BinomialDistribution binomial(20, 0.3);
    
    std::cout << "\nBinomial Distribution (n=20, p=0.3):\n";
    std::cout << "  Mean: " << (20 * 0.3) << "\n";
    std::cout << "  Variance: " << (20 * 0.3 * (1 - 0.3)) << "\n";
}

void demo_confidence_intervals() {
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Confidence Intervals                ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    NormalDistribution standard_normal(0.0, 1.0);
    
    std::cout << "Standard Normal Distribution (Z-scores):\n\n";
    
    // Common confidence levels
    std::vector<double> confidence_levels = {0.68, 0.90, 0.95, 0.99};
    
    std::cout << "Confidence Level  |  Z-score (±)\n";
    std::cout << "------------------+-------------\n";
    
    for (double conf : confidence_levels) {
        // For symmetric interval, find z such that P(-z < Z < z) = conf
        double tail_prob = (1 - conf) / 2;
        double z = 0.0;
        
        // Approximate z-score by searching
        for (double test_z = 0; test_z <= 3.0; test_z += 0.01) {
            if (standard_normal.cdf(test_z) >= (1 - tail_prob)) {
                z = test_z;
                break;
            }
        }
        
        std::cout << "      " << std::fixed << std::setprecision(2) << (conf * 100) 
                  << "%       |   ±" << std::setprecision(3) << z << "\n";
    }
}

int main() {
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════════════════╗\n";
    std::cout << "║                                                    ║\n";
    std::cout << "║       Machine Learning Demo                       ║\n";
    std::cout << "║       Probability Distributions                   ║\n";
    std::cout << "║                                                    ║\n";
    std::cout << "╚════════════════════════════════════════════════════╝\n";
    
    demo_normal_distribution();
    demo_binomial_distribution();
    demo_statistical_properties();
    demo_confidence_intervals();
    
    std::cout << "\n╔════════════════════════════════════════╗\n";
    std::cout << "║   Demo Complete!                      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    return 0;
}
