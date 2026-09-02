# v1.4.9 Linux Distributed Storage BlockId Array Include Fix

## Issue

Linux CI failed while compiling the `distributed_storage` target with errors in:

- `_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/block_store/headers/block_store.h`
- `_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/block_store/source/block_store.cpp`

Representative errors:

- `field 'id' has incomplete type 'BlockId'`
- `return type 'BlockId' is incomplete`
- multiple STL/vector/type_traits cascade failures involving `std::array<unsigned char, 32>`

## Root cause

`BlockId` is defined as:

- `using BlockId = std::array<std::uint8_t, 32>;`

but `block_store.h` did not include `<array>`. On Linux/GCC, that leaves `std::array` incomplete and causes a large compile error cascade.

## Fix

Updated:

- `_deliverables/libraries/groups/trekker/DISTRIBUTED_STORAGE/block_store/headers/block_store.h`

Change made:

- Added `#include <array>` at the top-level includes.

## Validation

Local target build completed successfully:

- `cmake --build build --target distributed_storage -j4`
- `BUILD_EXIT_CODE:0`

## Impact

- Restores Linux compilation for distributed storage block store code path.
- No runtime behavior changes; fix is a header completeness/include correctness update.
