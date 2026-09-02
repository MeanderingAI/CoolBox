#include "finance.hpp"

#include <tyst_framework.hpp>

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

TYST_TEST(FinanceTest, BlackScholesMatchesReferencePrices) {
    const double call_price = finance::black_scholes_call_price(100.0, 100.0, 0.05, 0.2, 1.0);
    const double put_price = finance::black_scholes_put_price(100.0, 100.0, 0.05, 0.2, 1.0);

    TYST_EXPECT_NEAR(call_price, 10.4505835722, 1e-6);
    TYST_EXPECT_NEAR(put_price, 5.5735260223, 1e-6);
    TYST_EXPECT_NEAR(call_price - put_price, 100.0 - (100.0 * std::exp(-0.05)), 1e-6);
}

TYST_TEST(FinanceTest, BlackScholesDeltasStayWithinBounds) {
    const double call_delta = finance::black_scholes_call_delta(100.0, 95.0, 0.03, 0.25, 1.5);
    const double put_delta = finance::black_scholes_put_delta(100.0, 95.0, 0.03, 0.25, 1.5);

    TYST_EXPECT_GT(call_delta, 0.0);
    TYST_EXPECT_LT(call_delta, 1.0);
    TYST_EXPECT_GT(put_delta, -1.0);
    TYST_EXPECT_LT(put_delta, 0.0);
    TYST_EXPECT_NEAR(call_delta - put_delta, 1.0, 1e-9);
}

TYST_TEST(FinanceTest, BlackScholesGreeksMatchReferenceValues) {
    TYST_EXPECT_NEAR(finance::black_scholes_gamma(100.0, 100.0, 0.05, 0.2, 1.0), 0.0187620173, 1e-8);
    TYST_EXPECT_NEAR(finance::black_scholes_vega(100.0, 100.0, 0.05, 0.2, 1.0), 37.5240346917, 1e-7);
    TYST_EXPECT_NEAR(finance::black_scholes_call_theta(100.0, 100.0, 0.05, 0.2, 1.0), -6.4140275464, 1e-7);
    TYST_EXPECT_NEAR(finance::black_scholes_put_theta(100.0, 100.0, 0.05, 0.2, 1.0), -1.6578804239, 1e-7);
    TYST_EXPECT_NEAR(finance::black_scholes_call_rho(100.0, 100.0, 0.05, 0.2, 1.0), 53.2324815454, 1e-7);
    TYST_EXPECT_NEAR(finance::black_scholes_put_rho(100.0, 100.0, 0.05, 0.2, 1.0), -41.8904609047, 1e-7);
}

TYST_TEST(FinanceTest, DigitalOptionParityMatchesDiscountFactor) {
    const double call_price = finance::black_scholes_digital_call_price(100.0, 100.0, 0.05, 0.2, 1.0);
    const double put_price = finance::black_scholes_digital_put_price(100.0, 100.0, 0.05, 0.2, 1.0);

    TYST_EXPECT_NEAR(call_price + put_price, std::exp(-0.05), 1e-9);
}

TYST_TEST(FinanceTest, GeometricAsianOptionsRespectParity) {
    const double call_price = finance::geometric_asian_call_price(100.0, 100.0, 0.05, 0.2, 1.0);
    const double put_price = finance::geometric_asian_put_price(100.0, 100.0, 0.05, 0.2, 1.0);
    const double discounted_geometric_forward = std::exp(-0.05 + std::log(100.0) + (0.05 - 0.02) * 0.5 + (0.04 / 6.0));

    TYST_EXPECT_GT(call_price, 0.0);
    TYST_EXPECT_GT(put_price, 0.0);
    TYST_EXPECT_NEAR(call_price - put_price, discounted_geometric_forward - 100.0 * std::exp(-0.05), 1e-9);
}

TYST_TEST(FinanceTest, DownBarrierCallParityMatchesVanillaCall) {
    const double vanilla_call = finance::black_scholes_call_price(100.0, 100.0, 0.05, 0.2, 1.0);
    const double down_in_call = finance::black_scholes_down_and_in_call_price(100.0, 100.0, 90.0, 0.05, 0.2, 1.0);
    const double down_out_call = finance::black_scholes_down_and_out_call_price(100.0, 100.0, 90.0, 0.05, 0.2, 1.0);

    TYST_EXPECT_GT(down_in_call, 0.0);
    TYST_EXPECT_GT(down_out_call, 0.0);
    TYST_EXPECT_NEAR(down_in_call + down_out_call, vanilla_call, 1e-8);
}

TYST_TEST(FinanceTest, BrownianMotionPathUsesSqrtTimeScaling) {
    const std::vector<double> path = finance::brownian_motion_path({0.25, 0.25, 0.25}, {0.0, 2.0, -1.0});

    TYST_ASSERT_EQ(path.size(), 4U);
    TYST_EXPECT_NEAR(path[0], 0.0, 1e-12);
    TYST_EXPECT_NEAR(path[1], 0.0, 1e-12);
    TYST_EXPECT_NEAR(path[2], 1.0, 1e-12);
    TYST_EXPECT_NEAR(path[3], 0.5, 1e-12);
}

TYST_TEST(FinanceTest, GeometricBrownianMotionReducesToExponentialGrowthWithoutVolatility) {
    const std::vector<double> path =
        finance::geometric_brownian_motion_path(100.0, 0.1, 0.0, {0.5, 0.5}, {0.0, 0.0});

    TYST_ASSERT_EQ(path.size(), 3U);
    TYST_EXPECT_NEAR(path[0], 100.0, 1e-12);
    TYST_EXPECT_NEAR(path[1], 100.0 * std::exp(0.05), 1e-12);
    TYST_EXPECT_NEAR(path[2], 100.0 * std::exp(0.1), 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloEuropeanCallUsesGbmPaths) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const double price = finance::monte_carlo_european_call_price(100.0, 100.0, 0.1, 0.0, {0.5, 0.5}, shock_paths);

    TYST_EXPECT_NEAR(price, 100.0 - 100.0 * std::exp(-0.1), 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloEstimateReportsZeroErrorForDeterministicPayoffs) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const finance::MonteCarloEstimate estimate =
        finance::monte_carlo_european_call_estimate(100.0, 100.0, 0.1, 0.0, {0.5, 0.5}, shock_paths);

    TYST_EXPECT_NEAR(estimate.price, 100.0 - 100.0 * std::exp(-0.1), 1e-12);
    TYST_EXPECT_NEAR(estimate.standard_error, 0.0, 1e-12);
    TYST_EXPECT_NEAR(estimate.confidence_interval_low, estimate.price, 1e-12);
    TYST_EXPECT_NEAR(estimate.confidence_interval_high, estimate.price, 1e-12);
    TYST_ASSERT_EQ(estimate.sample_count, 2U);
}

TYST_TEST(FinanceTest, MonteCarloEuropeanPutUsesGbmPaths) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const double price = finance::monte_carlo_european_put_price(100.0, 120.0, 0.0, 0.0, {0.5, 0.5}, shock_paths);

    TYST_EXPECT_NEAR(price, 20.0, 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloArithmeticAsianCallAveragesFixings) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const double price =
        finance::monte_carlo_arithmetic_asian_call_price(100.0, 90.0, 0.0, 0.0, {0.5, 0.5}, shock_paths);

    TYST_EXPECT_NEAR(price, 10.0, 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloDownAndOutCallKnocksOutWhenBarrierBreached) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const double live_price =
        finance::monte_carlo_down_and_out_call_price(100.0, 90.0, 80.0, 0.0, 0.0, {0.5, 0.5}, shock_paths);
    const double knocked_out_price =
        finance::monte_carlo_down_and_out_call_price(100.0, 90.0, 100.0, 0.0, 0.0, {0.5, 0.5}, shock_paths);

    TYST_EXPECT_NEAR(live_price, 10.0, 1e-12);
    TYST_EXPECT_NEAR(knocked_out_price, 0.0, 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloEuropeanGreeksProduceDeterministicFiniteDifferences) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const finance::MonteCarloGreeks greeks =
        finance::monte_carlo_european_call_greeks_fd(120.0, 100.0, 0.05, 0.0, {0.5, 0.5}, shock_paths, 1.959963984540054, 0.01, 0.01);

    TYST_EXPECT_NEAR(greeks.delta.price, 1.0, 1e-9);
    TYST_EXPECT_NEAR(greeks.gamma.price, 0.0, 1e-9);
    TYST_EXPECT_NEAR(greeks.rho.price, 100.0 * std::exp(-0.05), 1e-4);
    TYST_EXPECT_NEAR(greeks.theta.price, -5.0 * std::exp(-0.05), 1e-3);
    TYST_EXPECT_NEAR(greeks.delta.standard_error, 0.0, 1e-12);
}

TYST_TEST(FinanceTest, MonteCarloAsianGreeksSupportPathDependentPayoffs) {
    const std::vector<std::vector<double>> shock_paths = {{0.0, 0.0}, {0.0, 0.0}};
    const finance::MonteCarloGreeks greeks =
        finance::monte_carlo_arithmetic_asian_call_greeks_fd(120.0, 100.0, 0.0, 0.0, {0.5, 0.5}, shock_paths, 1.959963984540054, 0.01, 0.01);

    TYST_EXPECT_NEAR(greeks.delta.price, 1.0, 1e-9);
    TYST_EXPECT_NEAR(greeks.gamma.price, 0.0, 1e-9);
    TYST_EXPECT_NEAR(greeks.rho.price, 70.0, 1e-3);
    TYST_EXPECT_NEAR(greeks.theta.price, 0.0, 1e-9);
    TYST_EXPECT_NEAR(greeks.delta.standard_error, 0.0, 1e-12);
}

TYST_TEST(FinanceTest, ItoUtilitiesExposeIntegralAndVariance) {
    const double integral = finance::ito_integral_left_point({1.0, 2.0}, {0.1, -0.2});
    const double variance = finance::ito_isometry_variance({1.0, 2.0}, {0.25, 0.25});

    TYST_EXPECT_NEAR(integral, -0.3, 1e-12);
    TYST_EXPECT_NEAR(variance, 1.25, 1e-12);
}

TYST_TEST(FinanceTest, RejectsMismatchedPathInputs) {
    TYST_EXPECT_THROW(finance::brownian_motion_path({0.25}, {0.0, 1.0}), std::invalid_argument);
}

TYST_TEST(FinanceTest, RejectsEmptyMonteCarloShockCollection) {
    TYST_EXPECT_THROW(finance::monte_carlo_european_call_price(100.0, 100.0, 0.05, 0.2, {1.0}, {}),
                      std::invalid_argument);
}

// Up-barrier closed-form parity: up-in + up-out = vanilla call.
TYST_TEST(FinanceTest, UpBarrierCallParityMatchesVanillaCall) {
    // S=100, K=100, H=130 (barrier above spot and strike), r=0.05, sigma=0.2, T=1
    const double spot    = 100.0;
    const double strike  = 100.0;
    const double barrier = 130.0;
    const double rate    = 0.05;
    const double vol     = 0.2;
    const double T       = 1.0;

    const double vanilla   = finance::black_scholes_call_price(spot, strike, rate, vol, T);
    const double up_in     = finance::black_scholes_up_and_in_call_price(spot, strike, barrier, rate, vol, T);
    const double up_out    = finance::black_scholes_up_and_out_call_price(spot, strike, barrier, rate, vol, T);

    TYST_EXPECT_NEAR(up_in + up_out, vanilla, 1e-10);
    TYST_EXPECT_GE(up_in, 0.0);
    TYST_EXPECT_GT(up_out, 0.0);
}

// Down-barrier put parity: down-in + down-out = vanilla put.
TYST_TEST(FinanceTest, DownBarrierPutParityMatchesVanillaPut) {
    // S=100, K=100, H=70 (barrier below spot and strike), r=0.05, sigma=0.2, T=1
    const double spot    = 100.0;
    const double strike  = 100.0;
    const double barrier = 70.0;
    const double rate    = 0.05;
    const double vol     = 0.2;
    const double T       = 1.0;

    const double vanilla  = finance::black_scholes_put_price(spot, strike, rate, vol, T);
    const double down_in  = finance::black_scholes_down_and_in_put_price(spot, strike, barrier, rate, vol, T);
    const double down_out = finance::black_scholes_down_and_out_put_price(spot, strike, barrier, rate, vol, T);

    TYST_EXPECT_NEAR(down_in + down_out, vanilla, 1e-10);
    TYST_EXPECT_GE(down_in, 0.0);
    TYST_EXPECT_GT(down_out, 0.0);
}

// MC up-and-out call: knocks out when path exceeds barrier.
TYST_TEST(FinanceTest, MonteCarloUpAndOutCallKnocksOutWhenBarrierBreached) {
    // Two-step path with zero shocks: S stays at spot=100, K=90, H=101, live payoff=10.
    const double spot    = 100.0;
    const double strike  = 90.0;
    const double barrier = 101.0;
    const double rate    = 0.0;
    const double vol     = 0.001; // near zero

    const std::vector<double>              time_steps{{0.5, 0.5}};
    const std::vector<std::vector<double>> live_paths{{{0.0, 0.0}}};

    const double live_price = finance::monte_carlo_up_and_out_call_price(
        spot, strike, barrier, rate, vol, time_steps, live_paths);
    TYST_EXPECT_NEAR(live_price, 10.0, 0.01);

    // A large first shock crosses barrier=101 and knocks out the path.
    const std::vector<std::vector<double>> knocked_paths{{{100.0, 0.0}}};
    const double knocked_price = finance::monte_carlo_up_and_out_call_price(
        spot, strike, barrier, rate, vol, time_steps, knocked_paths);
    TYST_EXPECT_NEAR(knocked_price, 0.0, 1e-9);
}

// MC down-and-out put: knocks out when path drops below barrier.
TYST_TEST(FinanceTest, MonteCarloDownAndOutPutKnocksOutWhenBarrierBreached) {
    // S=100, K=110, H=99, r=0, vol~0 keeps the live path above barrier.
    const double spot    = 100.0;
    const double strike  = 110.0;
    const double barrier = 99.0;
    const double rate    = 0.0;
    const double vol     = 0.001;

    const std::vector<double>              time_steps{{0.5, 0.5}};
    const std::vector<std::vector<double>> live_paths{{{0.0, 0.0}}};

    const double live_price = finance::monte_carlo_down_and_out_put_price(
        spot, strike, barrier, rate, vol, time_steps, live_paths);
    TYST_EXPECT_NEAR(live_price, 10.0, 0.01);

    // A large negative first shock crosses below barrier=99 and knocks out the path.
    const std::vector<std::vector<double>> knocked_paths{{{-100.0, 0.0}}};
    const double knocked_price = finance::monte_carlo_down_and_out_put_price(
        spot, strike, barrier, rate, vol, time_steps, knocked_paths);
    TYST_EXPECT_NEAR(knocked_price, 0.0, 1e-9);
}

// MC barrier Greeks (FD) produce finite differences for down-and-out call.
TYST_TEST(FinanceTest, MonteCarloDownAndOutCallGreeksFdProduceFiniteDifferences) {
    // S=100, K=80, H=50 (barrier well below), r=0, vol~0 keeps paths alive.
    // Payoff = max(S-K,0) = 20 for zero shocks, so delta=1 and gamma=0.
    const double spot    = 100.0;
    const double strike  = 80.0;
    const double barrier = 50.0;
    const double rate    = 0.0;
    const double vol     = 0.001;

    const std::vector<double>              time_steps{{1.0}};
    const std::vector<std::vector<double>> paths{{{0.0}}, {{0.0}}};

    const finance::MonteCarloGreeks greeks = finance::monte_carlo_down_and_out_call_greeks_fd(
        spot, strike, barrier, rate, vol, time_steps, paths);

    TYST_EXPECT_NEAR(greeks.delta.price, 1.0, 0.01);
    TYST_EXPECT_NEAR(greeks.gamma.price, 0.0, 0.01);
}

// MC barrier Greeks (FD) produce finite differences for up-and-out call.
TYST_TEST(FinanceTest, MonteCarloUpAndOutCallGreeksFdProduceFiniteDifferences) {
    // S=100, K=80, H=200 (barrier well above), r=0, vol~0 keeps paths alive.
    const double spot    = 100.0;
    const double strike  = 80.0;
    const double barrier = 200.0;
    const double rate    = 0.0;
    const double vol     = 0.001;

    const std::vector<double>              time_steps{{1.0}};
    const std::vector<std::vector<double>> paths{{{0.0}}, {{0.0}}};

    const finance::MonteCarloGreeks greeks = finance::monte_carlo_up_and_out_call_greeks_fd(
        spot, strike, barrier, rate, vol, time_steps, paths);

    TYST_EXPECT_NEAR(greeks.delta.price, 1.0, 0.01);
    TYST_EXPECT_NEAR(greeks.gamma.price, 0.0, 0.01);
}

// Generic payoff interface prices a binary European call.
TYST_TEST(FinanceTest, GenericPayoffInterfacePricesBinaryEuropeanCall) {
    // Binary call: pays 1 if S_T > K, else 0
    // With vol~0, r=0 and S=100 > K=90, all paths end above strike so price ~= 1.
    const double spot    = 100.0;
    const double rate    = 0.0;
    const double vol     = 0.001;
    const double strike  = 90.0;

    const std::vector<double>              time_steps{{1.0}};
    const std::vector<std::vector<double>> paths{{{0.0}}, {{0.0}}};

    const finance::MonteCarloPayoff binary_call =
        [strike](const std::vector<double>& path) -> double {
        return path.back() > strike ? 1.0 : 0.0;
    };

    const finance::MonteCarloEstimate est = finance::monte_carlo_estimate_generic(
        spot, rate, vol, time_steps, paths, binary_call);

    TYST_EXPECT_NEAR(est.price, 1.0, 0.01);
    TYST_ASSERT_EQ(est.sample_count, 2U);

    // Greeks via generic FD: deep ITM binary remains insensitive to small spot bumps.
    const finance::MonteCarloGreeks greeks = finance::monte_carlo_greeks_fd_generic(
        spot, strike, rate, vol, time_steps, paths, binary_call);

    // Both bumped paths remain ITM, so the finite-difference delta stays near zero.
    TYST_EXPECT_NEAR(greeks.delta.price, 0.0, 0.01);
}

} // namespace
