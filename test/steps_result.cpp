///
/// Copyright 2026 Chris Solomon
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///

#include "cucumber.hpp"
#include "lish.h"

#include <string>

// ============================================================================
// RESULT TESTS (result.feature)
// ============================================================================
/// Tests for CommandResult status codes and success validation.

/// @brief Helper to convert string status names to ShellStatus enum values.
/// Parses status name strings like "Ok", "CommandNotFound", etc. for test parameterization.

static lish::ShellStatus string_to_status(const std::string& status_str) {
    if (status_str == "Ok") return lish::ShellStatus::Ok;
    if (status_str == "CommandNotFound") return lish::ShellStatus::CommandNotFound;
    if (status_str == "PermissionDenied") return lish::ShellStatus::PermissionDenied;
    if (status_str == "EmptyCommand") return lish::ShellStatus::EmptyCommand;
    return lish::ShellStatus::Ok;
}

struct ResultContext {
    lish::CommandResult result;
};

/// @brief Create a CommandResult with a specific status and exit code.
WHEN(when_create_result, "I create a result with status {string} and code {int}") {
    std::string status_str = CUKE_ARG(1);
    int code = CUKE_ARG(2);
    auto status = string_to_status(status_str);
    cuke::context<ResultContext>().result = lish::CommandResult{status, static_cast<int8_t>(code)};
}

/// @brief Verify the succeeded() status of a CommandResult (true only for Ok status with code 0).
THEN(then_result_succeeded, "the result succeeded status should be {word}") {
    std::string expected_str = CUKE_ARG(1);
    bool expected = (expected_str == "true");
    bool actual = cuke::context<ResultContext>().result.succeeded();
    cuke::equal(actual, expected);
}

