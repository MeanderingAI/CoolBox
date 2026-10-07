/**
 * @file test_memory.cpp
 * @brief Test suite for the Generics memory module.
 *
 * Tests cover:
 *   - RefCounted / RefCountedPtr intrusive reference counting
 *   - MemoryStats counters, reset, and the operator<< view override
 *   - TrackedAllocation per-type automatic allocation tracking
 *   - RefCountedBox / BoxedRefCountedPtr template-based (non-intrusive)
 *     reference counting
 */

#include <tyst_framework.hpp>

#include "boxed_ref_counted.hpp"
#include "memory_stats.hpp"
#include "ref_counted.hpp"
#include "tracked_allocation.hpp"

#include <sstream>
#include <string>
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

struct Gadget : public TrackedAllocation<Gadget> {
    int payload[8] = {};
};

// Combines the intrusive RefCounted counter with per-type TrackedAllocation
// allocation tracking - the two features compose via multiple inheritance.
class TrackedWidget : public RefCounted, public TrackedAllocation<TrackedWidget> {
public:
    explicit TrackedWidget(int value) : value_(value) {}
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

// ===================================================================
// TrackedAllocation tests
// ===================================================================

TYST_TEST(TrackedAllocationTest, NewAndDeleteFeedMemoryStatsAutomatically) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    Gadget* gadget = new Gadget();
    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(1));
    TYST_EXPECT_EQ(snap.current_bytes, sizeof(Gadget));

    delete gadget;
    snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(1));
    TYST_EXPECT_EQ(snap.current_bytes, static_cast<std::size_t>(0));

    stats.reset();
}

TYST_TEST(TrackedAllocationTest, ComposesWithIntrusiveRefCounted) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    {
        RefCountedPtr<TrackedWidget> ptr = make_ref_counted<TrackedWidget>(3);
        TYST_EXPECT_EQ(ptr->value(), 3);
        MemoryStatsSnapshot snap = stats.snapshot();
        TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(1));
    }

    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(1));
    TYST_EXPECT_EQ(snap.live_allocations, static_cast<std::size_t>(0));

    stats.reset();
}

// ===================================================================
// RefCountedBox / BoxedRefCountedPtr tests (template instead of inheritance)
// ===================================================================

TYST_TEST(BoxedRefCountedPtrTest, WorksWithBuiltInTypes) {
    auto ptr = make_boxed_ref_counted<int>(42);
    TYST_ASSERT_TRUE(static_cast<bool>(ptr));
    TYST_EXPECT_EQ(*ptr, 42);
    TYST_EXPECT_EQ(ptr.use_count(), 1);
}

TYST_TEST(BoxedRefCountedPtrTest, WorksWithThirdPartyTypesWithoutInheritance) {
    auto ptr = make_boxed_ref_counted<std::string>("hello");
    TYST_ASSERT_TRUE(static_cast<bool>(ptr));
    TYST_EXPECT_EQ(*ptr, "hello");
    ptr->append(" world");
    TYST_EXPECT_EQ(*ptr, "hello world");
}

TYST_TEST(BoxedRefCountedPtrTest, CopyIncrementsSharedCount) {
    auto first = make_boxed_ref_counted<int>(1);
    BoxedRefCountedPtr<int> second = first;
    TYST_EXPECT_EQ(first.use_count(), 2);
    TYST_EXPECT_EQ(second.use_count(), 2);
    TYST_EXPECT_EQ(*second, 1);
}

TYST_TEST(BoxedRefCountedPtrTest, MoveTransfersOwnershipWithoutRefCountChange) {
    auto first = make_boxed_ref_counted<int>(9);
    BoxedRefCountedPtr<int> second = std::move(first);
    TYST_EXPECT_FALSE(static_cast<bool>(first));
    TYST_ASSERT_TRUE(static_cast<bool>(second));
    TYST_EXPECT_EQ(second.use_count(), 1);
}

TYST_TEST(BoxedRefCountedPtrTest, AutomaticallyFeedsMemoryStats) {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    {
        auto ptr = make_boxed_ref_counted<int>(5);
        MemoryStatsSnapshot snap = stats.snapshot();
        TYST_EXPECT_EQ(snap.total_allocations, static_cast<std::size_t>(1));
    }

    MemoryStatsSnapshot snap = stats.snapshot();
    TYST_EXPECT_EQ(snap.total_deallocations, static_cast<std::size_t>(1));
    TYST_EXPECT_EQ(snap.live_allocations, static_cast<std::size_t>(0));

    stats.reset();
}
