#pragma once

#include <atomic>
#include <cstddef>
#include <ostream>

namespace generics::memory {

/// Point-in-time copy of the counters tracked by MemoryStats.
struct MemoryStatsSnapshot {
    std::size_t current_bytes = 0;      ///< Bytes currently allocated and not yet freed.
    std::size_t peak_bytes = 0;         ///< High-water mark of current_bytes.
    std::size_t live_allocations = 0;   ///< Allocations made but not yet freed.
    std::size_t total_allocations = 0;  ///< Lifetime count of allocations.
    std::size_t total_deallocations = 0; ///< Lifetime count of deallocations.
};

/// Process-wide, thread-safe allocation counters.
///
/// When the owning library is configured with GENERICS_MEMORY_ENABLE_STATS=ON,
/// global operator new/delete (see memory_stats.cpp) feed this singleton
/// automatically so every allocation in the process is tracked. The build
/// flag is optional and defaults to OFF because overriding global new/delete
/// affects the entire binary; with it off, MemoryStats still works and can
/// be driven manually via record_allocation()/record_deallocation(), e.g. by
/// custom allocators that want opt-in tracking.
class MemoryStats {
public:
    static MemoryStats& instance() noexcept;

    void record_allocation(std::size_t bytes) noexcept;
    void record_deallocation(std::size_t bytes) noexcept;

    MemoryStatsSnapshot snapshot() const noexcept;
    void reset() noexcept;

    /// operator<< override used to view/print the current statistics, e.g.
    /// `std::cout << generics::memory::MemoryStats::instance();`
    friend std::ostream& operator<<(std::ostream& os, const MemoryStats& stats);

private:
    MemoryStats() = default;
    MemoryStats(const MemoryStats&) = delete;
    MemoryStats& operator=(const MemoryStats&) = delete;

    std::atomic<std::size_t> current_bytes_{0};
    std::atomic<std::size_t> peak_bytes_{0};
    std::atomic<std::size_t> live_allocations_{0};
    std::atomic<std::size_t> total_allocations_{0};
    std::atomic<std::size_t> total_deallocations_{0};
};

/// True when this library was built with GENERICS_MEMORY_ENABLE_STATS=ON and
/// global operator new/delete are being tracked automatically.
#if defined(GENERICS_MEMORY_ENABLE_STATS)
inline constexpr bool kMemoryStatsTrackingEnabled = true;
#else
inline constexpr bool kMemoryStatsTrackingEnabled = false;
#endif

} // namespace generics::memory
