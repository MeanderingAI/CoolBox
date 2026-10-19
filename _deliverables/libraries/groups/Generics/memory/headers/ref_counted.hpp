#pragma once

#include <atomic>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace generics::memory {

/// Intrusive reference counter base class. Derive from this to make a type
/// usable with RefCountedPtr<T>. Each RefCounted object owns its own atomic
/// counter, so copies of a derived object start with a fresh count of zero
/// rather than inheriting the original's refcount.
class RefCounted {
public:
    RefCounted() noexcept = default;
    RefCounted(const RefCounted&) noexcept {}
    RefCounted& operator=(const RefCounted&) noexcept { return *this; }

    /// Number of RefCountedPtr instances currently sharing this object.
    long use_count() const noexcept { return ref_count_.load(std::memory_order_acquire); }

protected:
    virtual ~RefCounted() = default;

private:
    template <typename T>
    friend class RefCountedPtr;

    void add_ref() const noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

    /// Returns true once the count has dropped to zero, signalling the
    /// caller should destroy the object.
    bool release() const noexcept {
        return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }

    mutable std::atomic<long> ref_count_{0};
};

/// Intrusive, atomically reference-counted smart pointer for types deriving
/// from RefCounted. Unlike std::shared_ptr, the count lives inside the
/// pointee itself, so one RefCountedPtr can be constructed from raw
/// pointers/`this` without a separate control block.
template <typename T>
class RefCountedPtr {
public:
    constexpr RefCountedPtr() noexcept = default;
    constexpr RefCountedPtr(std::nullptr_t) noexcept {}

    /// Wraps a raw pointer. Set add_ref=false to adopt a pointer that was
    /// already created with a +1 count (e.g. returned from a factory).
    explicit RefCountedPtr(T* ptr, bool add_ref = true) noexcept : ptr_(ptr) {
        if (ptr_ && add_ref) {
            ptr_->add_ref();
        }
    }

    RefCountedPtr(const RefCountedPtr& other) noexcept : ptr_(other.ptr_) {
        if (ptr_) {
            ptr_->add_ref();
        }
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    RefCountedPtr(const RefCountedPtr<U>& other) noexcept : ptr_(other.get()) {
        if (ptr_) {
            ptr_->add_ref();
        }
    }

    RefCountedPtr(RefCountedPtr&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    ~RefCountedPtr() { release(); }

    RefCountedPtr& operator=(const RefCountedPtr& other) noexcept {
        RefCountedPtr(other).swap(*this);
        return *this;
    }

    RefCountedPtr& operator=(RefCountedPtr&& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    RefCountedPtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    void reset() noexcept {
        release();
        ptr_ = nullptr;
    }

    void reset(T* ptr, bool add_ref = true) noexcept {
        release();
        ptr_ = ptr;
        if (ptr_ && add_ref) {
            ptr_->add_ref();
        }
    }

    void swap(RefCountedPtr& other) noexcept { std::swap(ptr_, other.ptr_); }

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /// Current number of RefCountedPtr instances sharing the pointee, or 0
    /// if this pointer is empty.
    long use_count() const noexcept { return ptr_ ? ptr_->use_count() : 0; }

private:
    void release() noexcept {
        if (ptr_ && ptr_->release()) {
            delete ptr_;
        }
    }

    T* ptr_ = nullptr;
};

/// Allocates a new T and wraps it in a RefCountedPtr with a count of one.
template <typename T, typename... Args>
RefCountedPtr<T> make_ref_counted(Args&&... args) {
    return RefCountedPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename U>
bool operator==(const RefCountedPtr<T>& lhs, const RefCountedPtr<U>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T, typename U>
bool operator!=(const RefCountedPtr<T>& lhs, const RefCountedPtr<U>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T>
bool operator==(const RefCountedPtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const RefCountedPtr<T>& rhs) noexcept {
    return rhs.get() == nullptr;
}

template <typename T>
bool operator!=(const RefCountedPtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const RefCountedPtr<T>& rhs) noexcept {
    return rhs.get() != nullptr;
}

} // namespace generics::memory
