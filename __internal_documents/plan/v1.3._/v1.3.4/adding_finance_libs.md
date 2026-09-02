v1.3.4 plan: FINANCE library expansion in cool_car

Status
- Implemented and validated.
- Build target validated: finance_tests (Debug).
- Test result validated: 25 tests run, 25 passed, 0 failed.

Scope summary
- Extend FINANCE from core Black-Scholes utilities to a broader derivatives toolkit.
- Add closed-form barrier variants requested for this release cycle.
- Add Monte Carlo barrier pricing and finite-difference Greeks.
- Add a generic Monte Carlo payoff interface to avoid one-function-per-payoff API growth.

Files touched
- _deliverables/libraries/groups/cool_car/FINANCE/headers/finance.hpp
- _deliverables/libraries/groups/cool_car/FINANCE/source/finance.cpp
- _deliverables/libraries/groups/cool_car/FINANCE/tests/test_finance.cpp

Public API additions (finance.hpp)

Closed-form barrier pricing
- black_scholes_up_and_in_call_price(...)
- black_scholes_up_and_out_call_price(...)
- black_scholes_down_and_in_put_price(...)
- black_scholes_down_and_out_put_price(...)

Monte Carlo estimate and wrapper pricing
- monte_carlo_up_and_out_call_estimate(...)
- monte_carlo_down_and_out_put_estimate(...)
- monte_carlo_up_and_out_call_price(...)
- monte_carlo_down_and_out_put_price(...)

Monte Carlo barrier Greeks (finite differences)
- monte_carlo_down_and_out_call_greeks_fd(...)
- monte_carlo_up_and_out_call_greeks_fd(...)
- monte_carlo_down_and_out_put_greeks_fd(...)

Generic Monte Carlo payoff interface
- using MonteCarloPayoff = std::function<double(const std::vector<double>&)>;
- monte_carlo_estimate_generic(...)
- monte_carlo_greeks_fd_generic(...)

Implementation notes (finance.cpp)

Closed-form barriers
- Added up barrier call and down barrier put branches with barrier domain checks.
- Preserved in/out parity by deriving in values from vanilla - out.
- Added numerical stability clamp for out values into [0, vanilla] to prevent tiny negative artifacts from floating-point cancellation.

Monte Carlo barriers and Greeks
- Added path payoff helpers for up-and-out call and down-and-out put.
- Added Monte Carlo estimate functions for these path-dependent payoffs.
- Added finite-difference Greeks wrappers for barrier payoffs by reusing the existing internal finite-difference engine.

Generic payoff API
- Exposed a generic payoff type alias and estimate/Greek functions.
- Generic functions delegate to existing internal Monte Carlo estimate and finite-difference primitives.

Tests added/updated (test_finance.cpp)

New coverage added
- Up barrier call parity test: up-in + up-out equals vanilla call.
- Down barrier put parity test: down-in + down-out equals vanilla put.
- Monte Carlo up-and-out knockout behavior test.
- Monte Carlo down-and-out put knockout behavior test.
- Monte Carlo finite-difference Greeks test for down-and-out call.
- Monte Carlo finite-difference Greeks test for up-and-out call.
- Generic payoff interface test using a binary European call payoff.

Stability and correctness adjustments
- Knockout tests were adjusted to deterministic barrier-crossing setups.
- Barrier in-value positivity assertions were relaxed to non-negativity where numerically zero is valid under far-barrier settings.

Validation runbook
- Build command:
	cmake --build build --config Debug --target finance_tests
- Test command:
	build\_deliverables\libraries\groups\cool_car\FINANCE\Debug\finance_tests.exe
- Final observed result:
	25 passed, 0 failed.

Release notes bullets for v1.3.4
- Added closed-form up/down barrier option pricing extensions for calls/puts in FINANCE.
- Added Monte Carlo up/down barrier pricing APIs with confidence interval outputs.
- Added Monte Carlo finite-difference Greeks for barrier payoffs.
- Added generic Monte Carlo payoff and Greeks interfaces to simplify custom-derivative integration.
- Expanded FINANCE automated test coverage to 25 passing tests.

Follow-up candidates (post v1.3.4)
- Add remaining barrier family permutations for broader strike/barrier regimes.
- Add closed-form validations against external benchmark datasets.
- Add variance reduction options for Monte Carlo generic pricing (antithetic/control variates).