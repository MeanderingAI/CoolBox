#include "../include/timer_bindings.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <sstream>
#include <utility>

#include "timer.hpp"
#include "timing_stats.hpp"

namespace {

using generics::timer::ConcurrentTimingStats;
using generics::timer::Timer;
using generics::timer::TimingSnapshot;
using generics::timer::TimingStats;

class PythonScopedTimer {
public:
    explicit PythonScopedTimer(py::function callback)
        : callback_([callback = std::move(callback)](double elapsed_ms) {
              py::gil_scoped_acquire acquire;
              callback(elapsed_ms);
          }) {
        timer_.start();
    }

    explicit PythonScopedTimer(TimingStats& stats)
        : callback_([&stats](double elapsed_ms) {
              stats.record(std::chrono::duration<double, std::milli>(elapsed_ms));
          }) {
        timer_.start();
    }

    explicit PythonScopedTimer(ConcurrentTimingStats& stats)
        : callback_([&stats](double elapsed_ms) {
              stats.record(std::chrono::duration<double, std::milli>(elapsed_ms));
          }) {
        timer_.start();
    }

    PythonScopedTimer(const PythonScopedTimer&) = delete;
    PythonScopedTimer& operator=(const PythonScopedTimer&) = delete;

    ~PythonScopedTimer() noexcept {
        try {
            close();
        } catch (py::error_already_set& error) {
            error.discard_as_unraisable("timer.ScopedTimer");
        }
    }

    double elapsed_ms() const noexcept {
        return timer_.elapsed<std::chrono::duration<double, std::milli>>().count();
    }

    void close() {
        if (closed_) {
            return;
        }

        closed_ = true;
        timer_.stop();
        callback_(elapsed_ms());
    }

private:
    Timer timer_;
    std::function<void(double)> callback_;
    bool closed_ = false;
};

template <typename Stats, bool ReleaseGil>
void bind_stats(py::module_& module, const char* name, const char* description) {
    auto stats_class = py::class_<Stats>(module, name, description).def(py::init<>());
    if constexpr (ReleaseGil) {
        stats_class.def(
            "record_ns",
            [](Stats& stats, std::int64_t duration_ns) {
                stats.record(std::chrono::nanoseconds(duration_ns));
            },
            py::arg("duration_ns"),
            py::call_guard<py::gil_scoped_release>(),
            "Record one timing sample expressed in nanoseconds.");
    } else {
        stats_class.def(
            "record_ns",
            [](Stats& stats, std::int64_t duration_ns) {
                stats.record(std::chrono::nanoseconds(duration_ns));
            },
            py::arg("duration_ns"),
            "Record one timing sample expressed in nanoseconds.");
    }

    stats_class
        .def("snapshot", &Stats::snapshot, "Return a point-in-time copy of the statistics.")
        .def("reset", &Stats::reset, "Clear all recorded timing samples.")
        .def("__repr__", [](const Stats& stats) {
            std::ostringstream output;
            output << stats;
            return output.str();
        });
}

} // namespace

void bind_timer(py::module_& m) {
    py::module_ timer_module = m.def_submodule(
        "timer", "Monotonic timers and single- or multi-threaded timing statistics.");

    py::class_<Timer>(timer_module, "Timer", "Single-threaded monotonic stopwatch.")
        .def(py::init<>())
        .def("start", &Timer::start, "Start or resume timing.")
        .def("stop", &Timer::stop, "Stop timing and preserve the elapsed duration.")
        .def("reset", &Timer::reset, "Stop timing and reset the elapsed duration to zero.")
        .def(
            "lap_ms",
            [](Timer& timer) {
                return timer.lap<std::chrono::duration<double, std::milli>>().count();
            },
            "Return the elapsed milliseconds, reset the timer, and start the next lap.")
        .def(
            "elapsed_ms",
            [](const Timer& timer) {
                return timer.elapsed<std::chrono::duration<double, std::milli>>().count();
            },
            "Return elapsed milliseconds without stopping the timer.")
        .def_property_readonly("is_running", &Timer::is_running);

    py::class_<TimingSnapshot>(timer_module, "TimingSnapshot")
        .def_property_readonly("count", [](const TimingSnapshot& snapshot) {
            return snapshot.count;
        })
        .def_property_readonly("total_ns", [](const TimingSnapshot& snapshot) {
            return snapshot.total.count();
        })
        .def_property_readonly("min_ns", [](const TimingSnapshot& snapshot) {
            return snapshot.min.count();
        })
        .def_property_readonly("max_ns", [](const TimingSnapshot& snapshot) {
            return snapshot.max.count();
        })
        .def_property_readonly("mean_ns", &TimingSnapshot::mean_ns)
        .def("__repr__", [](const TimingSnapshot& snapshot) {
            std::ostringstream output;
            output << snapshot;
            return output.str();
        });

    bind_stats<TimingStats, false>(
        timer_module,
        "TimingStats",
        "Single-threaded timing-sample aggregator.");
    bind_stats<ConcurrentTimingStats, true>(
        timer_module,
        "ConcurrentTimingStats",
        "Thread-safe timing-sample aggregator.");

    py::class_<PythonScopedTimer>(timer_module, "ScopedTimer")
        .def(
            py::init<py::function>(),
            py::arg("callback"),
            "Start a scope timer that calls callback(elapsed_ms) when closed.")
        .def(
            py::init<TimingStats&>(),
            py::arg("stats"),
            py::keep_alive<1, 2>(),
            "Start a scope timer that records into single-threaded stats when closed.")
        .def(
            py::init<ConcurrentTimingStats&>(),
            py::arg("stats"),
            py::keep_alive<1, 2>(),
            "Start a scope timer that records into thread-safe stats when closed.")
        .def("elapsed_ms", &PythonScopedTimer::elapsed_ms)
        .def("close", &PythonScopedTimer::close, "Stop and report the sample exactly once.")
        .def(
            "__enter__",
            [](PythonScopedTimer& timer) -> PythonScopedTimer& {
                return timer;
            },
            py::return_value_policy::reference_internal)
        .def(
            "__exit__",
            [](PythonScopedTimer& timer, py::object, py::object, py::object) {
                timer.close();
                return false;
            });
}
