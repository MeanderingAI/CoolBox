# v1.4.8 Linux DataStructures MapleTree stdexcept Fix

## Issue

Linux CI failed while compiling DataStructures with errors from:

- `_deliverables/libraries/groups/trekker/DATASTRUCTURE/trees/headers/maple_tree.h`

Error summary:

- `'out_of_range' is not a member of 'std'`
- compiler note suggested missing include: `<stdexcept>`

## Root cause

`MapleTree::operator[]` throws `std::out_of_range`, but the header did not include `<stdexcept>`.

## Fix

Updated:

- `_deliverables/libraries/groups/trekker/DATASTRUCTURE/trees/headers/maple_tree.h`

Added:

- `#include <stdexcept>`

## Validation

Local target build completed successfully:

- `cmake --build build --target data_structures -j4`
- `BUILD_EXIT_CODE:0`

## Impact

- Restores Linux build for DataStructures/MapleTree compilation path.
- No behavior change beyond proper standard-library declaration visibility.
