#include "../headers/distributed_lock.h"

namespace distributed {

void DistributedLock::expire_if_needed() const {
    if (held_ && Clock::now() >= expiry_) {
        held_ = false;
        owner_.clear();
    }
}

bool DistributedLock::try_acquire(const std::string& owner, Duration lease) {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    if (held_) return false;
    held_   = true;
    owner_  = owner;
    expiry_ = Clock::now() + lease;
    cv_.notify_all();
    return true;
}

bool DistributedLock::acquire(const std::string& owner,
                               Duration           lease,
                               Duration           timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    const auto deadline = Clock::now() + timeout;

    while (true) {
        expire_if_needed();
        if (!held_) {
            held_   = true;
            owner_  = owner;
            expiry_ = Clock::now() + lease;
            return true;
        }
        if (cv_.wait_until(lock, deadline) == std::cv_status::timeout) {
            expire_if_needed();
            if (!held_) {
                held_   = true;
                owner_  = owner;
                expiry_ = Clock::now() + lease;
                return true;
            }
            return false;
        }
    }
}

bool DistributedLock::release(const std::string& owner) {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    if (!held_ || owner_ != owner) return false;
    held_ = false;
    owner_.clear();
    cv_.notify_all();
    return true;
}

bool DistributedLock::renew(const std::string& owner, Duration extra) {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    if (!held_ || owner_ != owner) return false;
    expiry_ += extra;
    return true;
}

bool DistributedLock::is_held() const {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    return held_;
}

std::string DistributedLock::current_owner() const {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    return held_ ? owner_ : std::string{};
}

std::int64_t DistributedLock::lease_remaining_ms() const {
    std::lock_guard<std::mutex> lock(mutex_);
    expire_if_needed();
    if (!held_) return 0;
    const auto rem = std::chrono::duration_cast<std::chrono::milliseconds>(
        expiry_ - Clock::now());
    return rem.count() > 0 ? rem.count() : 0;
}

} // namespace distributed
