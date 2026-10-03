#pragma once

/// Umbrella header for the Generics memory module: pulls in the intrusive
/// reference counter (ref_counted.hpp), the composition-based/template
/// reference counter (boxed_ref_counted.hpp), the memory statistics /
/// global operator new-delete overrides (memory_stats.hpp), and the
/// per-type automatic allocation tracking mixin (tracked_allocation.hpp).
#include "boxed_ref_counted.hpp"
#include "memory_stats.hpp"
#include "ref_counted.hpp"
#include "tracked_allocation.hpp"
