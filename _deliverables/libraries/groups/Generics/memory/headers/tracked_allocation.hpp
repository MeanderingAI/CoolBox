#pragma once

#include <cstdlib>
#include <cstddef>
#include <new>

#include "memory_stats.hpp"

namespace generics::memory {

/// CRTP mixin that gives Derived its own `operator new`/`operator delete`,
/// wired directly into MemoryStats.
///
/// This is the per-type counterpart to the global operator new/delete
/// overrides in memory_stats.cpp: those are opt-in via the
/// GENERICS_MEMORY_ENABLE_STATS build flag and, once enabled, affect every
/// allocation in the binary. TrackedAllocation<Derived> instead hooks the
/// allocation calls for exactly one type, automatically, with no build flag
/// at all - just derive from it:
///
/// \code
///   class Widget : public TrackedAllocation<Widget> { ... };
///
///   auto* w = new Widget();  // MemoryStats::instance() updated automatically
///   delete w;                // ditto
/// \endcode
///
/// As with any class-scope operator new/delete, correct bookkeeping relies
/// on delete being invoked through a pointer whose static type is Derived
/// (or a type with a virtual destructor whose final overrider is Derived's)
/// - the same requirement that applies to any custom per-class allocator,
/// not something specific to this mixin.
template <typename Derived>
class TrackedAllocation {
public:
    static void* operator new(std::size_t size) {
        void* ptr = allocate(size);
        MemoryStats::instance().record_allocation(size);
        return ptr;
    }

    static void* operator new[](std::size_t size) {
        void* ptr = allocate(size);
        MemoryStats::instance().record_allocation(size);
        return ptr;
    }

    static void operator delete(void* ptr, std::size_t size) noexcept {
        MemoryStats::instance().record_deallocation(size);
        std::free(ptr);
    }

    static void operator delete[](void* ptr, std::size_t size) noexcept {
        MemoryStats::instance().record_deallocation(size);
        std::free(ptr);
    }

protected:
    ~TrackedAllocation() = default;

private:
    // Deliberately goes through std::malloc rather than ::operator new:
    // if the global operator new/delete overrides in memory_stats.cpp are
    // also active (GENERICS_MEMORY_ENABLE_STATS=ON), calling ::operator new
    // here would resolve to that same override and double-count every
    // allocation. Routing through malloc keeps TrackedAllocation's counting
    // correct regardless of whether the global override is enabled.
    static void* allocate(std::size_t size) {
        void* ptr = std::malloc(size);
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }
};

} // namespace generics::memory
