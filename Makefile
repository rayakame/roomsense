# Development tasks. The firmware itself is built by PlatformIO, this file
# only wraps the commands so they are easy to remember.
#
#   make            build the firmware
#   make check      clang-format check + clang-tidy
#   make format     format all sources in place
#   make tidy-fix   apply clang-tidy fixes, then format
#
# Pass extra clang-tidy flags via TIDY_FLAGS, e.g.
#   make tidy TIDY_FLAGS="--warnings-as-errors='*'"

PIO ?= pio
ENV ?= feather_s3
CLANG_FORMAT ?= clang-format
TIDY_FLAGS ?=

# Files below src/ or include/ that should not be formatted or checked.
EXCLUDE :=

SOURCES := $(filter-out $(EXCLUDE),$(shell find src include -name '*.cpp' -o -name '*.h'))
CPP_SOURCES := $(filter %.cpp,$(SOURCES))

.PHONY: all build upload monitor clean format format-check tidy tidy-fix check

all: build

build:
	$(PIO) run -e $(ENV)

upload:
	$(PIO) run -e $(ENV) -t upload

monitor:
	$(PIO) device monitor

clean:
	$(PIO) run -e $(ENV) -t clean

format:
	$(CLANG_FORMAT) -i $(SOURCES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCES)

tidy:
	tools/clang_tidy.py --env $(ENV) --quiet $(TIDY_FLAGS) $(CPP_SOURCES)

tidy-fix:
	tools/clang_tidy.py --env $(ENV) --quiet --fix $(TIDY_FLAGS) $(CPP_SOURCES)
	$(MAKE) format

check: format-check tidy
