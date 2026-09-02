#ifndef DISTRIBUTED_LOCK_H
#define DISTRIBUTED_LOCK_H

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

namespace distributed {

// Lease-based distributed lock with configurable TTL and blocking acquire.
// Models the semantics of a single-node lock coordinator: a lock is held by
// at most one owner, expires after the lease duration, and can be renewed.
class DistributedLock {
public:
    using Clock    = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;

    // Attempt to acquire the lock without blocking. Returns true on success.
    bool try_acquire(const std::string& owner, Duration lease = Duration(5000));

    // Block until the lock is free or `timeout` elapses. Returns true if acquired.
    bool acquire(const std::string& owner,
                 Duration           lease   = Duration(5000),
                 Duration           timeout = Duration(2000));

    // Release the lock. Only the current owner may release. Returns true on success.
    bool release(const std::string& owner);

    // Extend the current lease by `extra`. Returns false if not held by owner.
    bool renew(const std::string& owner, Duration extra);

    bool          is_held()            const;
    std::string   current_owner()      const;
    // Returns milliseconds remaining on the lease, or 0 if expired/unowned.
    std::int64_t  lease_remaining_ms() const;

private:
    void expire_if_needed() const; // must be called under lock

    mutable std::mutex       mutex_;
    std::condition_variable  cv_;
    mutable std::string      owner_;
    mutable Clock::time_point expiry_;
    mutable bool             held_{false};
};

} // namespace distributed

#endif // DISTRIBUTED_LOCK_H
