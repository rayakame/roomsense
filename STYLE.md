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
`.clang-tidy`.

    # format everything
    clang-format -i $(find src include -name '*.h' -o -name '*.cpp')

    # run clang-tidy with the flags of the real build (all .cpp below src/)
    tools/clang_tidy.py

    # check single files, or let clang-tidy apply fixes
    tools/clang_tidy.py src/sensors/sgp41.cpp
    tools/clang_tidy.py --fix

`pio check` cannot be used: PlatformIO ships an old clang-tidy that does not
understand the toolchain's headers. `tools/clang_tidy.py` uses the system
clang-tidy with a rewritten `compile_commands.json` instead.
