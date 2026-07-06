#include <emscripten/bind.h>
#include "normal_distribution.h"
#include "bernoulli_distribution.h"
#include "binomial_distribution.h"
#include "poisson_distribution.h"
#include "exponential_distribution.h"
#include "gamma_distribution.h"

using namespace emscripten;

EMSCRIPTEN_BINDINGS(distribution_module) {
    class_<NormalDistribution>("NormalDistribution")
        .constructor<double, double>()
        .function("pdf", &NormalDistribution::pdf)
        .function("log_pdf", &NormalDistribution::log_pdf)
        .function("cdf", &NormalDistribution::cdf)
        .function("sample", &NormalDistribution::sample)
        .function("mean_function", &NormalDistribution::mean_function)
        .function("link_function", &NormalDistribution::link_function)
    ;

    class_<BernoulliDistribution>("BernoulliDistribution")
        .constructor<double>()
        .function("pdf", &BernoulliDistribution::pdf)
        .function("log_pdf", &BernoulliDistribution::log_pdf)
        .function("cdf", &BernoulliDistribution::cdf)
        .function("sample_discrete", &BernoulliDistribution::sample_discrete)
    ;

    class_<BinomialDistribution>("BinomialDistribution")
        .constructor<int, double>()
        .function("pdf", &BinomialDistribution::pdf)
        .function("log_pdf", &BinomialDistribution::log_pdf)
        .function("cdf", &BinomialDistribution::cdf)
        .function("sample_discrete", &BinomialDistribution::sample_discrete)
    ;

    class_<PoissonDistribution>("PoissonDistribution")
        .constructor<double>()
        .function("pdf", &PoissonDistribution::pdf)
        .function("log_pdf", &PoissonDistribution::log_pdf)
        .function("cdf", &PoissonDistribution::cdf)
        .function("sample_discrete", &PoissonDistribution::sample_discrete)
    ;

    class_<ExponentialDistribution>("ExponentialDistribution")
        .constructor<double>()
        .function("pdf", &ExponentialDistribution::pdf)
        .function("log_pdf", &ExponentialDistribution::log_pdf)
        .function("cdf", &ExponentialDistribution::cdf)
        .function("sample", &ExponentialDistribution::sample)
    ;

    class_<GammaDistribution>("GammaDistribution")
        .constructor<double, double>()
        .function("pdf", &GammaDistribution::pdf)
        .function("log_pdf", &GammaDistribution::log_pdf)
        .function("cdf", &GammaDistribution::cdf)
        .function("sample", &GammaDistribution::sample)
    ;
}
