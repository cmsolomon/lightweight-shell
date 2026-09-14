cmake_minimum_required(VERSION 3.15)
project(LightweightShell)

# ============================================================================
# Coverage Configuration (must be before add_subdirectory) - not optional
# ============================================================================

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(FATAL_ERROR "Coverage is only supported with GCC or Clang")
endif()

# Add coverage flags to compiler BEFORE compiling any targets
set(COVERAGE_FLAGS "--coverage")
string(APPEND CMAKE_CXX_FLAGS " ${COVERAGE_FLAGS}")
string(APPEND CMAKE_C_FLAGS " ${COVERAGE_FLAGS}")
string(APPEND CMAKE_EXE_LINKER_FLAGS " ${COVERAGE_FLAGS}")
string(APPEND CMAKE_SHARED_LINKER_FLAGS " ${COVERAGE_FLAGS}")

# Find lcov and genhtml for reporting - required, not optional
find_program(LCOV lcov)
find_program(GENHTML genhtml)
if(NOT LCOV)
    message(FATAL_ERROR "lcov not found. Install with: sudo apt-get install lcov")
endif()
if(NOT GENHTML)
    message(FATAL_ERROR "genhtml not found. Install with: sudo apt-get install lcov")
endif()

# Enable testing (but we'll create a custom test target that includes coverage)
enable_testing()

# Add test directory (compiles src/*.cpp directly; there is no standalone src/ library target)
add_subdirectory(test)

# Optional: set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# ============================================================================
# Custom Make Targets
# ============================================================================

# clean_except_libs: wipe build artifacts but keep cwt-cucumber library
# This allows faster rebuilds since the cucumber library (which takes time to build) is preserved
add_custom_target(clean_except_libs
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/test
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/bin
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/src
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/coverage.info
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning build artifacts (preserving cwt-cucumber libs)..."
)


# ============================================================================
# Arduino CLI Targets - Multi-Board Support
# ============================================================================

# Find arduino-cli executable
find_program(ARDUINO_CLI_EXECUTABLE arduino-cli
  HINTS
    ~/bin
    ~/.local/bin
    /usr/local/bin
    /usr/bin
  DOC "Path to arduino-cli executable"
)

if(NOT ARDUINO_CLI_EXECUTABLE)
  message(FATAL_ERROR "arduino-cli not found! Install with: curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh")
endif()

message(STATUS "Found arduino-cli: ${ARDUINO_CLI_EXECUTABLE}")

# Define supported board types
set(ARDUINO_BOARD_PRESET "uno" CACHE STRING "Arduino board preset")
set_property(CACHE ARDUINO_BOARD_PRESET PROPERTY STRINGS "uno" "uno_r4" "uno_r4_minima" "mega" "nano_esp32" "xiao_esp32c5" "custom")

# Board type mapping (includes default serial ports for common cases)
if(ARDUINO_BOARD_PRESET STREQUAL "uno")
  set(DEFAULT_BOARD "arduino:avr:uno")
  set(BOARD_DESCRIPTION "Arduino Uno (AVR)")
  set(REQUIRED_CORE "arduino:avr")
  set(DEFAULT_PORT "/dev/ttyUSB0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "uno_r4")
  set(DEFAULT_BOARD "arduino:renesas_uno:unor4wifi")
  set(BOARD_DESCRIPTION "Arduino Uno R4 WiFi (ARM/Renesas)")
  set(REQUIRED_CORE "arduino:renesas_uno")
  set(DEFAULT_PORT "/dev/ttyACM0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "uno_r4_minima")
  set(DEFAULT_BOARD "arduino:renesas_uno:minima")
  set(BOARD_DESCRIPTION "Arduino Uno R4 Minima (ARM/Renesas)")
  set(REQUIRED_CORE "arduino:renesas_uno")
  set(DEFAULT_PORT "/dev/ttyACM0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "mega")
  set(DEFAULT_BOARD "arduino:avr:mega")
  set(BOARD_DESCRIPTION "Arduino Mega 2560 (AVR)")
  set(REQUIRED_CORE "arduino:avr")
  set(DEFAULT_PORT "/dev/ttyUSB0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "nano_esp32")
  set(DEFAULT_BOARD "arduino:esp32:nano_nora")
  set(BOARD_DESCRIPTION "Arduino Nano ESP32 (Xtensa/ESP32-S3)")
  set(REQUIRED_CORE "arduino:esp32")
  set(DEFAULT_PORT "/dev/ttyACM0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "xiao_esp32c5")
  # esp32:esp32 (Espressif's own core), not arduino:esp32 (Arduino's own
  # core, above) - arduino:esp32 is still on ESP-IDF 2.0.18 and doesn't
  # support the C5 chip; esp32:esp32 3.x does.
  set(DEFAULT_BOARD "esp32:esp32:XIAO_ESP32C5")
  set(BOARD_DESCRIPTION "Seeed Studio XIAO ESP32C5 (RISC-V)")
  set(REQUIRED_CORE "esp32:esp32")
  set(DEFAULT_PORT "/dev/ttyACM0")
elseif(ARDUINO_BOARD_PRESET STREQUAL "custom")
  set(DEFAULT_BOARD "")
  set(BOARD_DESCRIPTION "Custom (must specify ARDUINO_BOARD_ID)")
  set(REQUIRED_CORE "")
  set(DEFAULT_PORT "/dev/ttyUSB0")
else()
  message(FATAL_ERROR "Unknown ARDUINO_BOARD_PRESET: ${ARDUINO_BOARD_PRESET}")
endif()

# Use board-specific port default (users can override with -DARDUINO_PORT if needed)
set(DETECTED_PORT "${DEFAULT_PORT}")
message(STATUS "Arduino: Using board-specific default port ${DETECTED_PORT}")

# Allow override via -DARDUINO_PORT=/dev/ttyACM0 or similar
# Only update port if user didn't explicitly specify it (check if it differs from preset default).
# custom is exempt from that heuristic: it has no board-specific DETECTED_PORT to fall back to
# (DEFAULT_PORT is just a placeholder for it), and an arbitrary custom board's real port can
# legitimately equal one of the hardcoded /dev/ttyUSB0 / /dev/ttyACM0 literals below - forcing
# it back to the placeholder in that case would silently discard a correct user-supplied port.
if(ARDUINO_BOARD_PRESET STREQUAL "custom" AND DEFINED ARDUINO_PORT AND NOT ARDUINO_PORT STREQUAL "")
  set(ARDUINO_PORT "${ARDUINO_PORT}" CACHE STRING "Arduino serial port")
elseif(NOT DEFINED ARDUINO_PORT OR ARDUINO_PORT STREQUAL "/dev/ttyUSB0" OR ARDUINO_PORT STREQUAL "/dev/ttyACM0")
  # User didn't specify a custom port, use board default
  set(ARDUINO_PORT "${DETECTED_PORT}" CACHE STRING "Arduino serial port" FORCE)
else()
  # User specified a custom port, keep it
  set(ARDUINO_PORT "${ARDUINO_PORT}" CACHE STRING "Arduino serial port")
endif()

# Force update ARDUINO_BOARD_ID based on current preset (FORCE prevents cache from blocking
# updates) - except for custom, which has no DEFAULT_BOARD of its own: forcing it here would
# discard the -DARDUINO_BOARD_ID the user is required to pass for this preset.
if(ARDUINO_BOARD_PRESET STREQUAL "custom")
  if(NOT DEFINED ARDUINO_BOARD_ID OR ARDUINO_BOARD_ID STREQUAL "")
    message(FATAL_ERROR "ARDUINO_BOARD_PRESET=custom requires -DARDUINO_BOARD_ID=arduino:arch:board")
  endif()
  set(ARDUINO_BOARD_ID "${ARDUINO_BOARD_ID}" CACHE STRING "Arduino board ID (e.g., arduino:avr:uno)")
  set(BOARD_DESCRIPTION "Custom (${ARDUINO_BOARD_ID})")
else()
  set(ARDUINO_BOARD_ID "${DEFAULT_BOARD}" CACHE STRING "Arduino board ID (e.g., arduino:avr:uno)" FORCE)
endif()
set(ARDUINO_LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Arduino libraries directory (contains lish/src/lish.h)")
set(ARDUINO_EXAMPLE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/examples/BasicShell")

# Build Arduino example (with map file to build directory)
add_custom_target(arduino_build
  COMMAND ${ARDUINO_CLI_EXECUTABLE} compile -b ${ARDUINO_BOARD_ID} --libraries ${ARDUINO_LIBS_DIR} --build-property compiler.c.elf.extra_flags="-Wl,-Map=${CMAKE_BINARY_DIR}/sketch.map" ${ARDUINO_EXAMPLE_DIR}
  COMMAND ${CMAKE_COMMAND} -E echo "Map file: ${CMAKE_BINARY_DIR}/sketch.map"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Building Arduino example for ${BOARD_DESCRIPTION}..."
)

# Upload to board
add_custom_target(arduino_run
  COMMAND ${ARDUINO_CLI_EXECUTABLE} upload -b ${ARDUINO_BOARD_ID} -p ${ARDUINO_PORT} ${ARDUINO_EXAMPLE_DIR}
  DEPENDS arduino_build
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  COMMENT "Uploading to Arduino on ${ARDUINO_PORT} (${BOARD_DESCRIPTION})..."
)

# Print configuration summary
message(STATUS "")
message(STATUS "Lightweight Shell Build System")
message(STATUS "================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Build commands:")
message(STATUS "  cmake --build .                            - Build the host-side test executables (not the Arduino sketch)")
message(STATUS "  cmake --build . --target clean             - Clean all build artifacts")
message(STATUS "  cmake --build . --target clean_except_libs - Clean (preserve cwt-cucumber libs for faster rebuild)")
message(STATUS "  ctest --output-on-failure                  - Run BDD tests, generate test report and coverage report (only prints test output on failure)")
message(STATUS "  cmake --build . --target arduino_build     - Compile Arduino example sketch")
message(STATUS "  cmake --build . --target arduino_run       - Compile and upload to Arduino")
message(STATUS "")
message(STATUS "Arduino configuration:")
message(STATUS "  Selected board: ${BOARD_DESCRIPTION}")
message(STATUS "  Board ID: ${ARDUINO_BOARD_ID}")
message(STATUS "  Serial port: ${ARDUINO_PORT}")
message(STATUS "  arduino-cli: ${ARDUINO_CLI_EXECUTABLE}")
message(STATUS "")
if(REQUIRED_CORE)
  message(STATUS "Required core: ${REQUIRED_CORE}")
  message(STATUS "  Install with: ${ARDUINO_CLI_EXECUTABLE} core install ${REQUIRED_CORE}")
  message(STATUS "")
endif()
message(STATUS "Switch Arduino boards:")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=uno")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=uno_r4          # WiFi variant")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=uno_r4_minima   # Minima variant")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=mega")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=nano_esp32      # Xtensa/ESP32-S3")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=xiao_esp32c5    # RISC-V/ESP32-C5")
message(STATUS "")
message(STATUS "Advanced options:")
message(STATUS "  cmake .. -DARDUINO_BOARD_PRESET=custom -DARDUINO_BOARD_ID=arduino:arch:board")
message(STATUS "  cmake .. -DARDUINO_PORT=/dev/ttyACM0")
message(STATUS "")
