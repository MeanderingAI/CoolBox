/**
 * @file test_memory.cpp
 * @brief Test suite for the Generics memory module.
 *
 * Tests cover:
 *   - RefCounted / RefCountedPtr intrusive reference counting
 *   - MemoryStats counters, reset, and the operator<< view override
 */

#include <tyst_framework.hpp>

#include "memory_stats.hpp"
#include "ref_counted.hpp"

#include <sstream>
#include <utility>

using namespace generics::memory;

namespace {

int g_destroy_count = 0;

class Widget : public RefCounted {
public:
    explicit Widget(int value) : value_(value) {}
    ~Widget() override { ++g_destroy_count; }

    int value() const { return value_; }

private:
    int value_;
};

} // namespace

// ===================================================================
// RefCounted / RefCountedPtr tests
// ===================================================================

TYST_TEST(RefCountedPtrTest, StartsEmpty) {
    RefCountedPtr<Widget> ptr;
    TYST_EXPECT_FALSE(static_cast<bool>(ptr));
    TYST_EXPECT_EQ(ptr.get(), nullptr);
    TYST_EXPECT_EQ(ptr.use_count(), 0);
}

TYST_TEST(RefCountedPtrTest, MakeRefCountedOwnsOneReference) {
    auto ptr = make_ref_counted<Widget>(42);
    TYST_ASSERT_TRUE(static_cast<bool>(ptr));
    TYST_EXPECT_EQ(ptr->value(), 42);
    TYST_EXPECT_EQ(ptr.use_count(), 1);
}

TYST_TEST(RefCountedPtrTest, CopyIncrementsSharedCount) {
    g_destroy_count = 0;
    auto first = make_ref_counted<Widget>(1);
    {
        RefCountedPtr<Widget> second = first;
        TYST_EXPECT_EQ(first.use_count(), 2);
        TYST_EXPECT_EQ(second.use_count(), 2);
        TYST_EXPECT_EQ(second->value(), 1);
    }
    TYST_EXPECT_EQ(first.use_count(), 1);
    TYST_EXPECT_EQ(g_destroy_count, 0);
}

TYST_TEST(RefCountedPtrTest, ReleasesWhenLastReferenceDrops) {
    g_destroy_count = 0;
    {
        auto first = make_ref_counted<Widget>(7);
        RefCountedPtr<Widget> second = first;
        first.reset();
        TYST_EXPECT_EQ(g_destroy_count, 0);
        TYST_EXPECT_EQ(second.use_count(), 1);
    }
    TYST_EXPECT_EQ(g_destroy_count, 1);
}

TYST_TEST(RefCountedPtrTest, MoveTransfersOwnershipWithoutRefCountChange) {
    auto first = make_ref_counted<Widget>(9);
    RefCountedPtr<Widget> second = std::move(first);
    TYST_EXPECT_FALSE(static_cast<bool>(first));
    TYST_ASSERT_TRUE(static_cast<bool>(second));
    TYST_EXPECT_EQ(second.use_count(), 1);
}

TYST_TEST(RefCountedPtrTest, EqualityComparesPointee) {
    auto first = make_ref_counted<Widget>(5);
    RefCountedPtr<Widget> second = first;
    RefCountedPtr<Widget> empty;

    TYST_EXPECT_TRUE(first == second);
    TYST_EXPECT_TRUE(first != empty);
    TYST_EXPECT_TRUE(empty == nullptr);
    TYST_EXPECT_TRUE(first != nullptr);
}

// ===================================================================
// MemoryStats tests
// ===================================================================

TYST_TEST(MemoryStatsTest, RecordAllocationTracksCurrentPeakAndTotals) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    stats.record_allocation(100);
    stats.record_allocation(50);

    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.current_bytes, static_cast<std::size_t>(150));
    TYST_EXPECT_EQ(snap.peak_bytes, static_cast<std::size_t>(150));
    TYST_EXPECT_EQ(snap.live_allocations, static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(2));
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(0));

    stats.reset();
}

TYST_TEST(MemoryStatsTest, RecordDeallocationKeepsPeakButDropsCurrent) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    stats.record_allocation(200);
    stats.record_deallocation(200);

    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.current_bytes, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.peak_bytes, static_cast<std::size_t>(200));
    TYST_EXPECT_EQ(snap.live_allocations, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(1));
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(1));

    stats.reset();
}

TYST_TEST(MemoryStatsTest, ResetZerosAllCounters) {
    MemoryStats& stats = MemoryStats::instance();
    stats.record_allocation(64);
    stats.reset();

    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.current_bytes, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.peak_bytes, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.live_allocations, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(0));
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(0));
}

TYST_TEST(MemoryStatsTest, StreamOperatorRendersSnapshot) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();
    stats.record_allocation(32);

    std::ostringstream out;
    out << stats;

    const std::string rendered = out.str();
    TYST_EXPECT_NE(rendered.find("current_bytes=32"), std::string::npos);
    TYST_EXPECT_NE(rendered.find("peak_bytes=32"), std::string::npos);

    stats.reset();
}
