# Summary of Changes: #pragma once → include guards

This document summarizes the codebase-wide change from `#pragma once` to lower_snake_case `#ifndef`/`#define`/`#endif` include guards.

## Change Description

- All header files previously using `#pragma once` now use a unique, lower_snake_case macro for their include guard.
- The macro is based on the file name, e.g., `#ifndef bayesian_network_h` for `bayesian_network.h`.
- This improves portability and avoids issues with non-standard `#pragma once` support on some compilers.

## Example Before/After

**Before:**
```cpp
#pragma once
```

**After:**
```cpp
#ifndef bayesian_network_h
#define bayesian_network_h
// ...header content...
#endif // bayesian_network_h
```

## Rationale
- Ensures compatibility with all C/C++ compilers.
- Follows best practices for header file protection.
- Macro names are now consistent and easy to read.

See `include_guard_file_list.md` for a full list of updated files.