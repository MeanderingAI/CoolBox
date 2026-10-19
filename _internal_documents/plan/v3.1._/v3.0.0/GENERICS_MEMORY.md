Adding an intrusive reference counter and memory statistics to Generics

## Summary

Added a `memory` module to `Generics` providing an intrusive, atomically
reference-counted smart pointer and a process-wide memory statistics
singleton, gated behind an optional CMake build flag. Followed up with two
more pieces: `TrackedAllocation<Derived>`, a per-type mixin that automates
`MemoryStats` tracking directly off a type's own `operator new`/`delete`
calls without needing the global build flag, and `RefCountedBox<T>` /
`BoxedRefCountedPtr<T>`, a composition-based reference counter that takes T
as a template argument instead of requiring inheritance from `RefCounted`.

## New module (`_deliverables/libraries/groups/Generics/memory/`)

- `generics::memory::RefCounted` / `generics::memory::RefCountedPtr<T>` — an
  intrusive, atomically reference-counted smart pointer. The count lives on
  the object itself (via the `RefCounted` base class) rather than in a
  separate control block, so a raw `T*`/`this` can be safely adopted into a
  `RefCountedPtr` without the double-control-block/double-free risk that
  comes with wrapping the same raw pointer in two separate `std::shared_ptr`s.
  Includes `make_ref_counted<T>(...)` for allocation + wrapping in one step.
- `generics::memory::MemoryStats` — a thread-safe singleton tracking
  current/peak bytes and live/total allocation counts, with an `operator<<`
  override for viewing a formatted snapshot
  (e.g. `std::cout << MemoryStats::instance();`).
- Optional `GENERICS_MEMORY_ENABLE_STATS` CMake build flag (default `OFF`)
  that, when enabled, overrides the global `operator new`/`operator delete`
  so every allocation in the binary automatically feeds `MemoryStats`.
  Left off by default since it affects the entire binary; `MemoryStats`
  remains usable manually (`record_allocation`/`record_deallocation`) even
  when the flag is off.
- `generics::memory::TrackedAllocation<Derived>` — a CRTP mixin giving
  `Derived` its own `operator new`/`operator delete`, wired directly into
  `MemoryStats`. Unlike the global override, this needs no build flag at
  all: derive a type from it and every allocation/deallocation of that type
  is tracked automatically, scoped to just that type.
- `generics::memory::RefCountedBox<T>` / `BoxedRefCountedPtr<T>` — the same
  single-allocation reference counting as `RefCountedPtr<T>`, but passed in
  via template argument instead of inheritance, so it also works on
  built-in types (`int`) and third-party types (`std::string`) that can't
  derive from `RefCounted`. Every box is allocated through
  `TrackedAllocation` internally, so it also feeds `MemoryStats`
  automatically with no build flag required.
- `generics_memory_tests` (tyst framework, 17 tests) covering `RefCountedPtr`
  copy/move/release/equality semantics, `MemoryStats` accumulation, peak
  tracking, reset, and the `operator<<` view, `TrackedAllocation`
  (standalone and composed with `RefCounted`), and `RefCountedBox`/
  `BoxedRefCountedPtr` (built-in types, third-party types, copy/move, and
  automatic `MemoryStats` feeding).

## Why intrusive?

A reference-counting scheme is **intrusive** when the count lives *inside
the object being counted* (as a member of the type itself), rather than in
a separate, out-of-line control block that a smart pointer allocates on its
own. In `ref_counted.hpp`, the count is a member of the `RefCounted` base
class:

```cpp
class RefCounted {
    ...
    mutable std::atomic<long> ref_count_{0};   // the count lives ON the object
};
```

Any type that wants to be shared via `RefCountedPtr<T>` must derive from
`RefCounted` (e.g. `class Widget : public RefCounted { ... };`), so the
counter is baked into `Widget`'s own memory layout — it "intrudes" into the
object, hence *intrusive* reference counting. This is in contrast to
**non-intrusive** schemes like `std::shared_ptr<T>`, where `T` doesn't need
to know or care about reference counting at all — `shared_ptr` allocates a
separate heap block (the "control block") next to/pointing at the object to
hold the count.

### Comparison against `std::shared_ptr`

| | Intrusive (`RefCountedPtr<T>` here) | Non-intrusive (`std::shared_ptr<T>`) |
|---|---|---|
| Count storage | Inside the object (`RefCounted` base) | Separate control block |
| Construct from raw `T*` or `this` | Safe — one object, one count, always consistent | Dangerous — two separately-constructed `shared_ptr`s from the same raw pointer create *two* control blocks and will double-free |
| Extra allocation | None (no control block) | One extra allocation per object (unless using `make_shared`) |
| Getting a smart pointer from inside a member function (`this`) | Natural (just wrap `this`) | Needs `enable_shared_from_this` machinery |
| Cost to adopt an already-counted object (e.g. returned by a C-style factory) | Cheap, just wrap the pointer | Not really possible without extra bookkeeping |

Practical reasons for choosing intrusive here:

1. **No double control-block, double-free footguns** — since the count is
   part of the object, there's only ever one source of truth for how many
   references exist, no matter how many times/places a raw `T*` gets
   wrapped.
2. **No extra heap allocation** for a control block — `make_ref_counted<T>()`
   does a single `new T(...)`, the counter is already part of that one
   allocation.
3. It mirrors a very common C/C++ interop pattern (COM `IUnknown`-style
   `AddRef`/`Release`, Qt's `QSharedData`, `boost::intrusive_ptr`), which is
   useful when objects cross API boundaries as raw pointers and need to be
   "picked up" into a smart pointer later (the
   `explicit RefCountedPtr(T* ptr, bool add_ref = true)` constructor is
   built exactly for that "adopt an existing +1 count" case).

The tradeoff is the one visible constraint: the type must publicly inherit
from `RefCounted`, so it only works for your own types — you can't make
`int` or `std::string` intrusively ref-counted without wrapping them in a
struct that derives from `RefCounted`. If ref-counting for arbitrary/
third-party types is needed instead, `std::shared_ptr` (non-intrusive) is
the right tool, which is why both idioms exist side by side rather than one
replacing the other. `RefCountedBox<T>` below closes this gap without
giving up the single-allocation/no-control-block property.

## Automating stats without the global flag: `TrackedAllocation<Derived>`

The global `operator new`/`operator delete` overrides (behind
`GENERICS_MEMORY_ENABLE_STATS`) automate `MemoryStats` for literally every
allocation in the binary, but that's an all-or-nothing switch. For a more
surgical option, `tracked_allocation.hpp` adds `TrackedAllocation<Derived>`,
a CRTP mixin that gives `Derived` its *own* `operator new`/`operator delete`
hooked directly to `MemoryStats::record_allocation`/`record_deallocation` —
no build flag required, and it only affects that one type:

```cpp
class Widget : public TrackedAllocation<Widget> { ... };

auto* w = new Widget();  // MemoryStats::instance() updated automatically
delete w;                // ditto
```

It composes with `RefCounted` via multiple inheritance, so a single type can
get both intrusive ref-counting *and* automatic tracking:

```cpp
class TrackedWidget : public RefCounted, public TrackedAllocation<TrackedWidget> { ... };
```

One subtlety worth calling out: `TrackedAllocation` intentionally allocates
via `std::malloc`/`std::free` rather than `::operator new`/`::operator
delete`. If it called the global `operator new` and the global override was
*also* enabled, the allocation would be recorded twice — once by
`TrackedAllocation` and once more by the global override it calls into.
Routing through `malloc` keeps the two features independent and safely
composable, enabled together or separately.

## Template-based reference counting without inheritance: `RefCountedBox<T>`

`RefCountedPtr<T>` requires `T` to derive from `RefCounted`, which doesn't
work for built-in types or third-party types you don't own. `RefCountedBox<T>`
(in `boxed_ref_counted.hpp`) gets the same single-allocation,
no-control-block behavior via composition instead: it stores a `T value_`
and its own atomic count side by side, with `T` supplied purely as a
template argument:

```cpp
auto n = make_boxed_ref_counted<int>(42);          // built-in type
auto s = make_boxed_ref_counted<std::string>("hi"); // third-party type
```

`BoxedRefCountedPtr<T>` mirrors `RefCountedPtr<T>`'s copy/move/reset
semantics, dereferencing through the boxed value instead of a count stored
on `T` itself. `RefCountedBox<T>` also derives from
`TrackedAllocation<RefCountedBox<T>>`, so every box allocated/freed feeds
`MemoryStats` automatically — demonstrating the two new pieces together:
automatic tracking with no build flag, and reference counting with no
inheritance requirement on `T`.

## Example usage

### `RefCountedPtr<T>`

```cpp
#include "ref_counted.hpp"

using namespace generics::memory;

class Widget : public RefCounted {
public:
    explicit Widget(int value) : value_(value) {}
    int value() const { return value_; }

private:
    int value_;
};

void example() {
    // Allocates a Widget and wraps it with a refcount of 1.
    RefCountedPtr<Widget> a = make_ref_counted<Widget>(42);

    // Copying bumps the shared count; both point at the same Widget.
    RefCountedPtr<Widget> b = a;
    assert(a.use_count() == 2);
    assert(b->value() == 42);

    // Moving transfers ownership without touching the count.
    RefCountedPtr<Widget> c = std::move(b);
    assert(!b);                 // b is now empty
    assert(c.use_count() == 2); // still shared with a

    // Last reference going out of scope (or reset()) destroys the Widget.
    a.reset();
    c.reset();
    // Widget is deleted here once the final RefCountedPtr releases it.
}
```

### `MemoryStats`

```cpp
#include "memory_stats.hpp"
#include <iostream>

using namespace generics::memory;

void example() {
    MemoryStats& stats = MemoryStats::instance();
    stats.reset();

    // Either fed automatically by the global operator new/delete overrides
    // (when built with -DGENERICS_MEMORY_ENABLE_STATS=ON), or driven
    // manually by a custom allocator that wants opt-in tracking:
    stats.record_allocation(128);
    stats.record_deallocation(64);

    MemoryStatsSnapshot snap = stats.snapshot();
    std::cout << "current=" << snap.current_bytes
              << " peak="    << snap.peak_bytes
              << " live="    << snap.live_allocations
              << '\n';

    // operator<< override for a quick formatted view:
    std::cout << stats << '\n';
    // -> MemoryStats{current_bytes=64, peak_bytes=128, live_allocations=0,
    //                total_allocations=1, total_deallocations=1}
}
```

Building with the optional flag enabled makes every `new`/`delete` in the
binary feed `MemoryStats` automatically, with no code changes required:

```sh
cmake -S . -B build -DGENERICS_MEMORY_ENABLE_STATS=ON
cmake --build build --target generics_memory_tests
```

### `TrackedAllocation<Derived>`

```cpp
#include "tracked_allocation.hpp"
#include "memory_stats.hpp"
#include <iostream>

using namespace generics::memory;

struct Gadget : public TrackedAllocation<Gadget> {
    int payload[8];
};

void example() {
    MemoryStats::instance().reset();

    Gadget* g = new Gadget();                 // tracked automatically
    std::cout << MemoryStats::instance() << '\n';
    // -> MemoryStats{current_bytes=32, peak_bytes=32, live_allocations=1,
    //                total_allocations=1, total_deallocations=0}

    delete g;                                 // tracked automatically
    std::cout << MemoryStats::instance() << '\n';
    // -> MemoryStats{current_bytes=0, peak_bytes=32, live_allocations=0,
    //                total_allocations=1, total_deallocations=1}
}
```

### `RefCountedBox<T>` / `BoxedRefCountedPtr<T>`

```cpp
#include "boxed_ref_counted.hpp"
#include <string>

using namespace generics::memory;

void example() {
    // Works on a built-in type - no inheritance possible or needed.
    BoxedRefCountedPtr<int> a = make_boxed_ref_counted<int>(42);
    BoxedRefCountedPtr<int> b = a;             // shares the same box
    assert(a.use_count() == 2);
    assert(*b == 42);

    // Works on a third-party type the same way.
    auto s = make_boxed_ref_counted<std::string>("hello");
    s->append(" world");
    assert(*s == "hello world");

    // MemoryStats::instance() was already updated for both boxes above,
    // with no build flag and no inheritance from RefCounted required.
}
```
