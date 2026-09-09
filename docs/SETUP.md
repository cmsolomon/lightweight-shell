<p align="center">
  <img src="assets/lish.svg" alt="lish" width="80">
</p>

# Development Setup

Building lish itself and running its BDD test suite - for contributors,
not for using the library in your own sketch (see
[Getting Started](GETTING_STARTED.md) for that).

## Prerequisites

```bash
./install-prerequisites.sh
```

Detects your OS (Ubuntu/Debian, Fedora/RHEL/CentOS, or macOS) and installs
everything needed to build and test lish: CMake, a C++ compiler, Boost,
Google Test, and `nlohmann_json` (all of which `cwt-cucumber`, the BDD
framework the test suite runs on, needs), plus `arduino-cli` and the board
cores for every preset this project's `CMakeLists.txt` supports, and adds
you to the `dialout` group so uploading to a board doesn't fail on a
permissions error. Safe to re-run - it skips anything already installed.

## Building

```bash
cmake -S . -B build
cmake --build build -j $(nproc)
```

The first build takes a couple of minutes - `cwt-cucumber` (the BDD
framework) is fetched and built from source via CMake's `FetchContent`,
pinned to a specific release tag for reproducibility. Subsequent builds
are incremental and fast, as long as `build/` isn't deleted.

`-j $(nproc)` isn't optional ceremony - it's what makes that first build
tolerable. `cmake --build` accepts it directly and works the same way
regardless of which generator is behind it (Makefiles, Ninja, ...), unlike
`make -j$(nproc)`, which only works if you happen to be on the Makefiles
generator.

## Running Tests

```bash
cd build
ctest --output-on-failure
```

`--output-on-failure` controls how much you see per test: a passing test
just prints a one-line `Passed` summary, but a failing one also prints
everything that test wrote to stdout/stderr - the actual assertion
failures, not just "failed." Without the flag, a failure still fails the
run, but you get no diagnostic output to explain why, only the bare
pass/fail line - so it's worth always including, not something to reach
for only when something's already gone wrong.

Running `ctest` alone does three things, in order, every time - not just
the BDD suite:

1. **`LightweightShellBDD`** - the actual test run, all `features/*.feature`
   files against `test/steps_*.cpp`, and writes `build/results.json`
   (cwt-cucumber's own JSON report format) as a side effect.
2. **`GenerateTestReport`** - renders `results.json` into
   `build/test_report.html`. Runs regardless of whether the BDD test
   above passed or failed - a failing run still gets a report.
3. **`GenerateCoverageReport`** - runs `gcov`/`lcov`/`genhtml` and writes
   `build/coverage/index.html`. Also runs unconditionally - coverage from
   a failing run is still informative. This one isn't optional: `lcov` and
   `genhtml` are required at configure time (a `FATAL_ERROR` if either is
   missing, not a silent skip), and coverage instrumentation flags are
   always on. `install-prerequisites.sh` installs both.

`ctest` doesn't build anything itself - if you skip the `cmake --build`
step above, or edit source and forget to rebuild, `ctest` will happily run
a stale or missing binary rather than catching that for you. Run the build
step first, every time.

To run just the BDD executable directly (skipping the report/coverage
steps, e.g. while iterating on a single scenario):

```bash
cd build
./test/lish_tests ../features/*.feature
```

Or to filter to one feature file:

```bash
./test/lish_tests ../features/tabcomplete.feature
```

## Test Reports

After `ctest --output-on-failure`, three files exist under `build/`:

| File | What it is |
|---|---|
| `results.json` | Raw cwt-cucumber JSON report |
| `test_report.html` | Human-readable HTML report, generated from `results.json` by `test/generate_report.cpp` (a small C++ tool built alongside the tests - no Node.js or other external tooling involved) |
| `coverage/index.html` | Line/function coverage report from `lcov`/`genhtml` |

```bash
open build/test_report.html        # macOS
xdg-open build/test_report.html    # Linux
```

## Coverage

Coverage instrumentation (`--coverage` compiler/linker flags) is always
on for this project - there's no `ENABLE_COVERAGE` toggle to flip off.
`ctest` regenerates `build/coverage/index.html` on every run (see
[Running Tests](#running-tests) above); there's no separate script or
target for it.

## Arduino Build

```bash
cmake --build build --target arduino_build
```

Compiles `examples/BasicShell` via `arduino-cli` for whichever board is
currently selected (Arduino Uno by default). To upload it to a connected
board:

```bash
cmake --build build --target arduino_run
```

### Switching boards

```bash
cmake -S . -B build -DARDUINO_BOARD_PRESET=uno_r4        # Uno R4 WiFi
cmake -S . -B build -DARDUINO_BOARD_PRESET=uno_r4_minima # Uno R4 Minima
cmake -S . -B build -DARDUINO_BOARD_PRESET=mega           # Mega 2560
```

Each preset has its own default serial port (`/dev/ttyUSB0` for AVR boards,
`/dev/ttyACM0` for the R4 boards); override it explicitly if needed:

```bash
cmake -S . -B build -DARDUINO_PORT=/dev/ttyACM1
```

For a board with no built-in preset, use `custom` and supply the full
board ID yourself:

```bash
cmake -S . -B build -DARDUINO_BOARD_PRESET=custom -DARDUINO_BOARD_ID=arduino:arch:board
```

## Cleaning

```bash
cmake --build build --target clean          # Standard clean
cmake --build build --target clean_except_libs   # Clean, but keep cwt-cucumber built
```

The second form is the one worth knowing about: it wipes `test/`, `bin/`,
`src/`, `CMakeFiles/`, and the coverage output, but leaves the
already-built `cwt-cucumber` library (`build/lib/libcucumber*.a`) alone -
since that's the slow part of a clean build, a subsequent build recompiles
only your own code, not `cwt-cucumber`/`gtest`, cutting a clean rebuild
from minutes to seconds.

It also deletes `CMakeCache.txt` (and `cmake_install.cmake`), so - unlike
a plain `clean` - you must reconfigure before building again:

```bash
cmake --build build --target clean_except_libs
cmake -S . -B build
cmake --build build -j $(nproc)
```

For a fully clean slate (including re-fetching `cwt-cucumber` itself),
just delete the build directory and reconfigure:

```bash
rm -rf build
cmake -S . -B build
```

## Project Structure

```
lish/
├── src/                       # The library itself
│   ├── lish.h                 # Aggregating header + make_shell()
│   ├── lishShell.h / .ipp     # Shell - owns everything
│   ├── lishInputProcessor.h / .ipp
│   ├── lishLineBuffer.h
│   ├── lishLineEditor.h
│   ├── lishHistory.h
│   ├── lishTabComplete.h / .cpp
│   ├── lishCmdDescriptor.h / .cpp
│   ├── lishCmdPermission.h
│   ├── lishHelp.h
│   ├── lishArgs.h / .cpp
│   ├── lishResult.h
│   ├── lishPlatform.h
│   └── lishDefs.h
├── test/                      # Host-side BDD test suite
│   ├── main.cpp                    # cwt-cucumber entry point
│   ├── steps_*.cpp                 # Step definitions, one file per feature
│   ├── steps_common.h              # Fixtures shared by more than one steps_*.cpp
│   ├── shared_test_io.h            # SharedTestIOAdapter (shell.feature + input_processor.feature)
│   ├── generate_report.cpp         # HTML report generator
│   └── CMakeLists.txt
├── features/                  # Gherkin feature files (one per src/ area)
├── examples/BasicShell/       # Reference Arduino sketch
├── docs/                      # This documentation
│   └── assets/                     # SVG logos
├── cmake/                     # CMake script-mode helpers (e.g. coverage generation)
├── library.properties         # Arduino Library Manager metadata
├── install-prerequisites.sh
└── CMakeLists.txt             # Top-level: coverage config, custom targets, Arduino targets
```

## Troubleshooting

### "arduino-cli not found"

`arduino-cli` is a hard requirement at CMake-configure time (not just for
the `arduino_build`/`arduino_run` targets - even a host-only build fails
configure without it, since both are defined unconditionally in the
top-level `CMakeLists.txt`). Run `./install-prerequisites.sh`, or install
it manually from
[arduino.cc](https://arduino.github.io/arduino-cli/latest/installation/).

### "lcov not found" / "genhtml not found"

Both are required at configure time (coverage isn't optional - see
[Coverage](#coverage) above).

```bash
sudo apt-get install lcov      # Debian/Ubuntu
sudo dnf install lcov          # Fedora
brew install lcov              # macOS
```

### Tests hang or a stale process is still running

```bash
ps aux | grep lish_tests
pkill lish_tests
```

### Upload fails with a serial port permission error

You need to be in the `dialout` group (Linux); `install-prerequisites.sh`
adds you to it, but group membership only takes effect on your *next*
login - log out and back in, or run `newgrp dialout` in your current shell.
