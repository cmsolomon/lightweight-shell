cmake_minimum_required(VERSION 3.15)
project(LightweightShellTests)

# Fetch and build cwt-cucumber
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)

# Pinned to the 2.9 release tag rather than tracking main, for a reproducible
# build. Bump deliberately (and re-verify).
FetchContent_Declare(
    cwt-cucumber
    GIT_REPOSITORY https://github.com/ThoSe1990/cwt-cucumber.git
    GIT_TAG 2.9
)

FetchContent_MakeAvailable(cwt-cucumber)

# Fetch nlohmann/json (header-only) for the report generator
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

# BDD test executable
add_executable(lish_tests
  main.cpp
  steps_defs.cpp
  steps_permissions.cpp
  steps_descriptors.cpp
  steps_args.cpp
  steps_result.cpp
  steps_history.cpp
  steps_linebuffer.cpp
  steps_lineeditor.cpp
  steps_shell.cpp
  steps_tabcomplete.cpp
  steps_input_processor.cpp
  ../src/lishArgs.cpp
  ../src/lishCmdDescriptor.cpp
  ../src/lishTabComplete.cpp
)

target_include_directories(lish_tests PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(lish_tests PRIVATE
  cucumber
)

target_compile_features(lish_tests PRIVATE cxx_std_20)

# HTML test report generator (reads cwt-cucumber's --report-json output)
add_executable(report_generator generate_report.cpp)

target_link_libraries(report_generator PRIVATE nlohmann_json::nlohmann_json)

target_compile_features(report_generator PRIVATE cxx_std_17)

# Enable testing
enable_testing()
add_test(
  NAME LightweightShellBDD
  COMMAND lish_tests
    ${CMAKE_SOURCE_DIR}/features/defs.feature
    ${CMAKE_SOURCE_DIR}/features/permissions.feature
    ${CMAKE_SOURCE_DIR}/features/descriptors.feature
    ${CMAKE_SOURCE_DIR}/features/args.feature
    ${CMAKE_SOURCE_DIR}/features/result.feature
    ${CMAKE_SOURCE_DIR}/features/history.feature
    ${CMAKE_SOURCE_DIR}/features/linebuffer.feature
    ${CMAKE_SOURCE_DIR}/features/lineeditor.feature
    ${CMAKE_SOURCE_DIR}/features/shell.feature
    ${CMAKE_SOURCE_DIR}/features/tabcomplete.feature
    ${CMAKE_SOURCE_DIR}/features/input_processor.feature
    --report-json ${CMAKE_BINARY_DIR}/results.json
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Renders results.json (written by the test above) into an HTML report.
# DEPENDS only orders this after LightweightShellBDD - it still runs even if
# that test fails, since results.json is written on failure too.
add_test(
  NAME GenerateTestReport
  COMMAND report_generator
    ${CMAKE_BINARY_DIR}/results.json
    ${CMAKE_BINARY_DIR}/test_report.html
)
set_tests_properties(GenerateTestReport PROPERTIES DEPENDS LightweightShellBDD)

# Coverage is meaningful whether or not tests passed, so this always runs
# too (same DEPENDS-not-FIXTURES reasoning as above).
add_test(
  NAME GenerateCoverageReport
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${CMAKE_BINARY_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/GenerateCoverage.cmake
)
set_tests_properties(GenerateCoverageReport PROPERTIES DEPENDS LightweightShellBDD)

