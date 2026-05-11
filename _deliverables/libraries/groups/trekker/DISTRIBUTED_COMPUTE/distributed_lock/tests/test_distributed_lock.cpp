#include "tyst_framework.hpp"

#include "distributed_lock.h"

#include <chrono>
#include <thread>

using namespace distributed;

TEST(DistributedLockTest, TryAcquireSucceedsWhenFree) {
    DistributedLock lock;
    EXPECT_TRUE(lock.try_acquire("node-1"));
    EXPECT_TRUE(lock.is_held());
    EXPECT_EQ(lock.current_owner(), "node-1");
}

TEST(DistributedLockTest, TryAcquireFailsWhenHeld) {
    DistributedLock lock;
    EXPECT_TRUE(lock.try_acquire("node-1"));
    EXPECT_FALSE(lock.try_acquire("node-2"));
    EXPECT_EQ(lock.current_owner(), "node-1");
}

TEST(DistributedLockTest, OwnerCanRelease) {
    DistributedLock lock;
    lock.try_acquire("node-1");
    EXPECT_TRUE(lock.release("node-1"));
    EXPECT_FALSE(lock.is_held());
    EXPECT_EQ(lock.current_owner(), "");
}

TEST(DistributedLockTest, NonOwnerCannotRelease) {
    DistributedLock lock;
    lock.try_acquire("node-1");
    EXPECT_FALSE(lock.release("node-2"));
    EXPECT_TRUE(lock.is_held());
}

TEST(DistributedLockTest, LeaseAutoExpires) {
    DistributedLock lock;
    lock.try_acquire("node-1", std::chrono::milliseconds(40));
    std::this_thread::sleep_for(std::chrono::milliseconds(70));
    EXPECT_FALSE(lock.is_held());
    EXPECT_EQ(lock.current_owner(), "");
}

TEST(DistributedLockTest, RenewExtendsLease) {
    DistributedLock lock;
    lock.try_acquire("node-1", std::chrono::milliseconds(80));
    EXPECT_TRUE(lock.renew("node-1", std::chrono::milliseconds(5000)));
    EXPECT_GT(lock.lease_remaining_ms(), 1000LL);
}

TEST(DistributedLockTest, NonOwnerRenewFails) {
    DistributedLock lock;
    lock.try_acquire("node-1", std::chrono::milliseconds(5000));
    EXPECT_FALSE(lock.renew("node-2", std::chrono::milliseconds(1000)));
}

TEST(DistributedLockTest, BlockingAcquireSucceedsAfterRelease) {
    DistributedLock lock;
    lock.try_acquire("node-1");

    std::thread releaser([&] {
        std::this_thread::sleep_for(std::chrono::milliseconds(40));
        lock.release("node-1");
    });

    bool acquired = lock.acquire("node-2",
                                  std::chrono::milliseconds(5000),
                                  std::chrono::milliseconds(500));
    EXPECT_TRUE(acquired);
    EXPECT_EQ(lock.current_owner(), "node-2");
    lock.release("node-2");
    releaser.join();
}

TEST(DistributedLockTest, BlockingAcquireTimesOut) {
    DistributedLock lock;
    lock.try_acquire("node-1", std::chrono::milliseconds(5000));

    bool acquired = lock.acquire("node-2",
                                  std::chrono::milliseconds(5000),
                                  std::chrono::milliseconds(30));
    EXPECT_FALSE(acquired);
    lock.release("node-1");
}
