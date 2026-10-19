#include "memory_stats.hpp"

#include <cstdlib>
#include <new>

namespace generics::memory {

MemoryStats& MemoryStats::instance() noexcept {
    static MemoryStats stats;
    return stats;
}

void MemoryStats::record_allocation(std::size_t bytes) noexcept {
    total_allocations_.fetch_add(1, std::memory_order_relaxed);
    live_allocations_.fetch_add(1, std::memory_order_relaxed);

    const std::size_t updated = current_bytes_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
    std::size_t prev_peak = peak_bytes_.load(std::memory_order_relaxed);
    while (updated > prev_peak &&
           !peak_bytes_.compare_exchange_weak(prev_peak, updated, std::memory_order_relaxed)) {
        // prev_peak is refreshed by compare_exchange_weak on failure; retry.
    }
}

void MemoryStats::record_deallocation(std::size_t bytes) noexcept {
    total_deallocations_.fetch_add(1, std::memory_order_relaxed);
    live_allocations_.fetch_sub(1, std::memory_order_relaxed);
    current_bytes_.fetch_sub(bytes, std::memory_order_relaxed);
}

MemoryStatsSnapshot MemoryStats::snapshot() const noexcept {
    return MemoryStatsSnapshot{
        current_bytes_.load(std::memory_order_relaxed),
        peak_bytes_.load(std::memory_order_relaxed),
        live_allocations_.load(std::memory_order_relaxed),
        total_allocations_.load(std::memory_order_relaxed),
        total_deallocations_.load(std::memory_order_relaxed),
    };
}

void MemoryStats::reset() noexcept {
    current_bytes_.store(0, std::memory_order_relaxed);
    peak_bytes_.store(0, std::memory_order_relaxed);
    live_allocations_.store(0, std::memory_order_relaxed);
    total_allocations_.store(0, std::memory_order_relaxed);
    total_deallocations_.store(0, std::memory_order_relaxed);
}

std::ostream& operator<<(std::ostream& os, const MemoryStats& stats) {
    const MemoryStatsSnapshot snap = stats.snapshot();
    return os << "MemoryStats{"
              << "current_bytes=" << snap.current_bytes
              << ", peak_bytes=" << snap.peak_bytes
              << ", live_allocations=" << snap.live_allocations
              << ", total_allocations=" << snap.total_allocations
              << ", total_deallocations=" << snap.total_deallocations
              << "}";
}

} // namespace generics::memory

#if defined(GENERICS_MEMORY_ENABLE_STATS)

// Global operator new/delete overrides, compiled only when the owning
// CMakeLists.txt enables GENERICS_MEMORY_ENABLE_STATS. Each allocation
// stashes its requested size in a small header in front of the block so
// deallocation can report accurate byte counts regardless of whether the
// sized or unsized delete overload is the one invoked by the caller.
namespace {

constexpr std::size_t kBookkeepingSize = sizeof(std::size_t);

void* tracked_allocate(std::size_t size) {
    void* raw = std::malloc(size + kBookkeepingSize);
    if (!raw) {
        throw std::bad_alloc();
    }
    *static_cast<std::size_t*>(raw) = size;
    generics::memory::MemoryStats::instance().record_allocation(size);
    return static_cast<char*>(raw) + kBookkeepingSize;
}

void tracked_deallocate(void* ptr) noexcept {
    if (!ptr) {
        return;
    }
    void* raw = static_cast<char*>(ptr) - kBookkeepingSize;
    const std::size_t size = *static_cast<std::size_t*>(raw);
    generics::memory::MemoryStats::instance().record_deallocation(size);
    std::free(raw);
}

} // namespace

void* operator new(std::size_t size) {
    return tracked_allocate(size);
}

void* operator new[](std::size_t size) {
    return tracked_allocate(size);
}

void operator delete(void* ptr) noexcept {
    tracked_deallocate(ptr);
}

void operator delete[](void* ptr) noexcept {
    tracked_deallocate(ptr);
}

void operator delete(void* ptr, std::size_t /*size*/) noexcept {
    tracked_deallocate(ptr);
}

void operator delete[](void* ptr, std::size_t /*size*/) noexcept {
    tracked_deallocate(ptr);
}

#endif // GENERICS_MEMORY_ENABLE_STATS
