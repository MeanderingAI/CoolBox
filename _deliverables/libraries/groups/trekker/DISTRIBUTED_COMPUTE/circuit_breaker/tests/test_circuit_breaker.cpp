#include "tyst_framework.hpp"

#include "circuit_breaker.h"

#include <chrono>
#include <stdexcept>
#include <thread>

using namespace distributed;

TEST(CircuitBreakerTest, InitialStateIsClosed) {
    CircuitBreaker cb;
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.state_name(), "Closed");
}

TEST(CircuitBreakerTest, SuccessfulCallsKeepCircuitClosed) {
    CircuitBreaker cb;
    for (int i = 0; i < 20; ++i) cb.call([] {});
    EXPECT_EQ(cb.state(), CircuitState::Closed);
}

TEST(CircuitBreakerTest, FailuresOpenCircuitAtThreshold) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 3; ++i) {
        try { cb.call([] { throw std::runtime_error("fail"); }); } catch (...) {}
    }
    EXPECT_EQ(cb.state(), CircuitState::Open);
    EXPECT_EQ(cb.state_name(), "Open");
}

TEST(CircuitBreakerTest, OpenCircuitRejectsFastWithoutCallingFn) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 2; ++i) {
        try { cb.call([] { throw std::runtime_error("err"); }); } catch (...) {}
    }

    bool threw = false;
    try {
        cb.call([] {}); // should throw, fn should never execute
    } catch (const std::runtime_error&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
}

TEST(CircuitBreakerTest, CircuitTransitionsToHalfOpenAfterTimeout) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::milliseconds(50);
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 2; ++i) {
        try { cb.call([] { throw std::runtime_error("err"); }); } catch (...) {}
    }
    EXPECT_EQ(cb.state(), CircuitState::Open);

    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    cb.call([] {}); // first call after timeout transitions to HalfOpen then records success
    // After one success it should be HalfOpen (success_threshold default = 2)
    EXPECT_EQ(cb.state(), CircuitState::HalfOpen);
}

TEST(CircuitBreakerTest, HalfOpenClosesAfterEnoughSuccesses) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.success_threshold = 2;
    cfg.timeout           = std::chrono::milliseconds(40);
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 2; ++i) {
        try { cb.call([] { throw std::runtime_error("err"); }); } catch (...) {}
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    cb.call([] {}); // 1st success in HalfOpen
    cb.call([] {}); // 2nd success — should close
    EXPECT_EQ(cb.state(), CircuitState::Closed);
}

TEST(CircuitBreakerTest, HalfOpenFailureReopensCircuit) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::milliseconds(40);
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 2; ++i) {
        try { cb.call([] { throw std::runtime_error("err"); }); } catch (...) {}
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    // Probe fails — circuit should reopen
    try { cb.call([] { throw std::runtime_error("still failing"); }); } catch (...) {}
    EXPECT_EQ(cb.state(), CircuitState::Open);
}

TEST(CircuitBreakerTest, ResetRestoresClosedState) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 1;
    CircuitBreaker cb(cfg);

    try { cb.call([] { throw std::runtime_error("x"); }); } catch (...) {}
    EXPECT_EQ(cb.state(), CircuitState::Open);

    cb.reset();
    EXPECT_EQ(cb.state(), CircuitState::Closed);
    EXPECT_EQ(cb.failure_count(), 0u);
    EXPECT_EQ(cb.success_count(), 0u);
}

TEST(CircuitBreakerTest, FailureCountTracked) {
    CircuitBreakerConfig cfg;
    cfg.failure_threshold = 10;
    CircuitBreaker cb(cfg);

    for (int i = 0; i < 4; ++i) {
        try { cb.call([] { throw std::runtime_error("err"); }); } catch (...) {}
    }
    EXPECT_EQ(cb.failure_count(), 4u);
    EXPECT_EQ(cb.state(), CircuitState::Closed);
}
