#!/usr/bin/env python3
"""Runs clang-tidy on the project sources with the flags of the real build.

clang does not support the xtensa target, so PlatformIO's compile_commands.json
cannot be used as is. This script rewrites it: it drops flags clang does not
know, selects a 32-bit target with similar properties and adds the toolchain's
built-in include paths. The rewritten database is stored in .pio/clang-tidy/.

Usage:
    tools/clang_tidy.py [--env ENV] [--no-compiledb] [clang-tidy args] [files]

Without files all .cpp files below src/ are checked. --no-compiledb reuses the
existing compile_commands.json instead of regenerating it. All other arguments
are passed to clang-tidy unchanged, e.g. --fix.
"""

import json
import os
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DB_DIR = os.path.join(ROOT, ".pio", "clang-tidy")

# A 32-bit little-endian target. x86 also knows the asm register constraint
# "a" that ESP-IDF headers use for xtensa.
TARGET = "i386-none-elf"

# gcc flags clang rejects or warns about.
DROPPED_PREFIXES = (
    "-m",
    "-fno-tree-",
    "-freorder-blocks",
    "-fstrict-volatile-bitfields",
    "-MMD",
)

EXTRA_FLAGS = [
    f"--target={TARGET}",
    # Never fall back to the host's system headers.
    "-nostdlibinc",
    "-D__XTENSA__=1",
    "-D__xtensa__=1",
    "-Wno-unknown-warning-option",
    "-Wno-ignored-optimization-argument",
    "-Wno-ignored-attributes",
]


def run(cmd, **kwargs):
    return subprocess.run(cmd, cwd=ROOT, check=False, **kwargs)


def builtin_include_dirs(compiler):
    """Returns the include directories gcc adds implicitly."""
    result = run(
        [compiler, "-E", "-xc++", "-v", os.devnull],
        capture_output=True,
        text=True,
    )
    dirs = []
    collecting = False
    for line in result.stderr.splitlines():
        if line.startswith("#include <...> search starts here:"):
            collecting = True
        elif line.startswith("End of search list."):
            break
        elif collecting:
            path = os.path.realpath(line.strip())
            # clang ships its own versions of gcc's built-in headers
            # (stddef.h, stdint.h, ...); those must not be shadowed.
            if "/lib/gcc/" in path:
                continue
            dirs.append(path)
    return dirs


def rewrite_entry(entry, include_dirs):
    args = shlex.split(entry["command"]) if "command" in entry else list(entry["arguments"])
    compiler = args[0]
    new_args = ["clang++"]
    skip_next = False
    for arg in args[1:]:
        if skip_next:
            skip_next = False
            continue
        if arg in ("-o", "-c"):
            skip_next = arg == "-o"
            continue
        if arg.startswith(DROPPED_PREFIXES):
            continue
        new_args.append(arg)
    new_args[1:1] = EXTRA_FLAGS + [f"-isystem{d}" for d in include_dirs]
    return compiler, {
        "directory": entry["directory"],
        "file": entry["file"],
        "arguments": new_args,
    }


def build_database():
    with open(os.path.join(ROOT, "compile_commands.json"), encoding="utf-8") as f:
        db = json.load(f)

    include_dirs = None
    rewritten = []
    for entry in db:
        if not os.path.abspath(entry["file"]).startswith(os.path.join(ROOT, "src") + os.sep):
            continue
        compiler, new_entry = rewrite_entry(entry, include_dirs or [])
        if include_dirs is None:
            include_dirs = builtin_include_dirs(compiler)
            _, new_entry = rewrite_entry(entry, include_dirs)
        rewritten.append(new_entry)

    os.makedirs(DB_DIR, exist_ok=True)
    with open(os.path.join(DB_DIR, "compile_commands.json"), "w", encoding="utf-8") as f:
        json.dump(rewritten, f, indent=2)


def default_sources():
    sources = []
    for dirpath, _, filenames in os.walk(os.path.join(ROOT, "src")):
        sources.extend(
            os.path.join(dirpath, n) for n in filenames if n.endswith(".cpp")
        )
    return sorted(sources)


def main(argv):
    env = "feather_s3"
    compiledb = True
    tidy_args = []
    files = []
    it = iter(argv)
    for arg in it:
        if arg == "--env":
            env = next(it)
        elif arg == "--no-compiledb":
            compiledb = False
        elif os.path.isfile(arg):
            files.append(os.path.abspath(arg))
        else:
            tidy_args.append(arg)

    if compiledb or not os.path.isfile(os.path.join(ROOT, "compile_commands.json")):
        run(["pio", "run", "-e", env, "-t", "compiledb", "--silent"])
    build_database()

    cmd = ["clang-tidy", "-p", DB_DIR, *tidy_args, *(files or default_sources())]
    return run(cmd).returncode


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
