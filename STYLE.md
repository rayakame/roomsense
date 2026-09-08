# Code Style

The code follows the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
with these deviations:

- `#pragma once` instead of `#define` include guards.
- `.cpp` instead of `.cc` as source file extension.
- Line length 100 instead of 80 characters.

All code lives in `namespace roomsense`. Project headers are included with their
full path relative to `src/` (e.g. `#include "sensors/sensor.h"`).

## Tooling

Formatting is defined in `.clang-format`, static checks and naming rules in
`.clang-tidy`. The Makefile wraps both:

    make format          # format all sources in place
    make format-check    # non-zero exit if a file is not formatted
    make tidy            # clang-tidy on all .cpp files
    make tidy-fix        # apply clang-tidy fixes, then format
    make check           # format-check + tidy

`pio check` cannot be used: PlatformIO ships an old clang-tidy that does not
understand the toolchain's headers. `make tidy` runs `tools/clang_tidy.py`,
which uses the system clang-tidy with a rewritten `compile_commands.json`
instead. Single files can be checked with `tools/clang_tidy.py <file>`.
