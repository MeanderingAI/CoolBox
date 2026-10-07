#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

#include "tracked_allocation.hpp"

namespace generics::memory {

template <typename T>
class BoxedRefCountedPtr;

/// Composition-based reference counter: holds a T value and its own atomic
/// count side by side in a single allocation, the same way RefCounted does
/// it - but without requiring T to derive from anything. Where RefCounted
/// requires inheritance (`class Widget : public RefCounted`), RefCountedBox
/// takes T purely as a template argument, so it also works for built-in
/// types and third-party types you can't/won't modify:
///
/// \code
///   auto n = make_boxed_ref_counted<int>(42);          // built-in type
///   auto s = make_boxed_ref_counted<std::string>("hi"); // third-party type
/// \endcode
///
/// Every RefCountedBox is allocated/freed through TrackedAllocation, so
/// MemoryStats::instance() is updated automatically for boxed objects with
/// no build flag or global operator new/delete override required.
template <typename T>
class RefCountedBox : public TrackedAllocation<RefCountedBox<T>> {
public:
    template <typename... Args>
    explicit RefCountedBox(Args&&... args) : value_(std::forward<Args>(args)...) {}

    T& value() noexcept { return value_; }
    const T& value() const noexcept { return value_; }

private:
    template <typename U>
    friend class BoxedRefCountedPtr;

    void add_ref() const noexcept { ref_count_.fetch_add(1, std::memory_order_relaxed); }

    bool release() const noexcept {
        return ref_count_.fetch_sub(1, std::memory_order_acq_rel) == 1;
    }

    long use_count() const noexcept { return ref_count_.load(std::memory_order_acquire); }

    T value_;
    mutable std::atomic<long> ref_count_{0};
};

/// Smart pointer for RefCountedBox<T>. Same copy/move/reset semantics as
/// RefCountedPtr<T>, but dereferences through the boxed value rather than
/// requiring T itself to carry the count.
template <typename T>
class BoxedRefCountedPtr {
public:
    constexpr BoxedRefCountedPtr() noexcept = default;
    constexpr BoxedRefCountedPtr(std::nullptr_t) noexcept {}

    /// Wraps a raw box pointer. Set add_ref=false to adopt a box that was
    /// already created with a +1 count.
    explicit BoxedRefCountedPtr(RefCountedBox<T>* box, bool add_ref = true) noexcept : box_(box) {
        if (box_ && add_ref) {
            box_->add_ref();
        }
    }

    BoxedRefCountedPtr(const BoxedRefCountedPtr& other) noexcept : box_(other.box_) {
        if (box_) {
            box_->add_ref();
        }
    }

    BoxedRefCountedPtr(BoxedRefCountedPtr&& other) noexcept : box_(other.box_) {
        other.box_ = nullptr;
    }

    ~BoxedRefCountedPtr() { release(); }

    BoxedRefCountedPtr& operator=(const BoxedRefCountedPtr& other) noexcept {
        BoxedRefCountedPtr(other).swap(*this);
        return *this;
    }

    BoxedRefCountedPtr& operator=(BoxedRefCountedPtr&& other) noexcept {
        if (this != &other) {
            release();
            box_ = other.box_;
            other.box_ = nullptr;
        }
        return *this;
    }

    BoxedRefCountedPtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    void reset() noexcept {
        release();
        box_ = nullptr;
    }

    void swap(BoxedRefCountedPtr& other) noexcept { std::swap(box_, other.box_); }

    T* get() const noexcept { return box_ ? &box_->value() : nullptr; }
    T& operator*() const noexcept { return box_->value(); }
    T* operator->() const noexcept { return &box_->value(); }
    explicit operator bool() const noexcept { return box_ != nullptr; }

    /// Current number of BoxedRefCountedPtr instances sharing the box, or 0
    /// if this pointer is empty.
    long use_count() const noexcept { return box_ ? box_->use_count() : 0; }

private:
    void release() noexcept {
        if (box_ && box_->release()) {
            delete box_;
        }
    }

    RefCountedBox<T>* box_ = nullptr;
};

/// Allocates a new RefCountedBox<T> and wraps it in a BoxedRefCountedPtr
/// with a count of one.
template <typename T, typename... Args>
BoxedRefCountedPtr<T> make_boxed_ref_counted(Args&&... args) {
    return BoxedRefCountedPtr<T>(new RefCountedBox<T>(std::forward<Args>(args)...));
}

template <typename T>
bool operator==(const BoxedRefCountedPtr<T>& lhs, const BoxedRefCountedPtr<T>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T>
bool operator!=(const BoxedRefCountedPtr<T>& lhs, const BoxedRefCountedPtr<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template <typename T>
bool operator==(const BoxedRefCountedPtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() == nullptr;
}

template <typename T>
bool operator==(std::nullptr_t, const BoxedRefCountedPtr<T>& rhs) noexcept {
    return rhs.get() == nullptr;
}

template <typename T>
bool operator!=(const BoxedRefCountedPtr<T>& lhs, std::nullptr_t) noexcept {
    return lhs.get() != nullptr;
}

template <typename T>
bool operator!=(std::nullptr_t, const BoxedRefCountedPtr<T>& rhs) noexcept {
    return rhs.get() != nullptr;
}

} // namespace generics::memory
