# Generates the lcov/genhtml coverage report from .gcda files produced by
# the test run. Runs unconditionally - coverage from a failing run is still
# useful (it shows what a failure did or didn't exercise).
#
# Variables are passed via -D flags when called from CMake (see test/CMakeLists.txt).

find_program(LCOV lcov)
find_program(GENHTML genhtml)
find_program(GCOV gcov-13 gcov)

if(NOT (LCOV AND GENHTML AND GCOV))
    message(FATAL_ERROR "lcov/genhtml/gcov not found. Install with: sudo apt-get install lcov")
endif()

message(STATUS "Processing coverage data...")
execute_process(
    COMMAND bash -c "find . -name '*.gcda' | while read gcda; do dir=\\$(dirname \\$gcda); base=\\$(basename \\$gcda .gcda); (cd \\$dir && ${GCOV} \\$base.gcda > /dev/null 2>&1 || true); done"
    WORKING_DIRECTORY ${BINARY_DIR}
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND ${LCOV} --directory . --capture --output-file coverage.info --ignore-errors mismatch,gcov
    WORKING_DIRECTORY ${BINARY_DIR}
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND ${LCOV} --remove coverage.info "/usr/*" "*/test/*" "*/_deps/*" --output-file coverage.info
    WORKING_DIRECTORY ${BINARY_DIR}
    OUTPUT_QUIET
    ERROR_QUIET
)

execute_process(
    COMMAND ${GENHTML} --output-directory coverage coverage.info
    WORKING_DIRECTORY ${BINARY_DIR}
    RESULT_VARIABLE GENHTML_RESULT
    OUTPUT_QUIET
    ERROR_QUIET
)

if(NOT GENHTML_RESULT EQUAL 0)
    message(FATAL_ERROR "genhtml failed to generate the coverage report")
endif()

message(STATUS "Coverage report generated: ${BINARY_DIR}/coverage/index.html")
