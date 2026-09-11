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

#include <cstring>
#include <memory>
#include <string>

// ============================================================================
// ARGS TESTS (args.feature)
// ============================================================================

struct ArgsContext {
    static constexpr size_t MaxLineLength = 256;
    char line_buffer_[MaxLineLength];
    std::unique_ptr<lish::Args> args;
    lish::Args::Terminator last_terminator;

    ArgsContext() {
        std::memset(line_buffer_, 0, MaxLineLength);
        last_terminator = lish::Args::Terminator::EndOfLine;
    }
};

GIVEN(given_command_string, "I have a command string {string}") {
    auto& ctx = cuke::context<ArgsContext>();
    std::string cmd = CUKE_ARG(1);
    size_t len = cmd.length();
    if (len >= ArgsContext::MaxLineLength) {
        len = ArgsContext::MaxLineLength - 1;
    }
    for (size_t i = 0; i < len; ++i) {
        ctx.line_buffer_[i] = cmd[i];
    }
    ctx.line_buffer_[len] = '\0';
}

WHEN(when_parse_command_line, "I parse the command line") {
    auto& ctx = cuke::context<ArgsContext>();

    // Parse from buffer and store Args object for THEN steps
    ctx.args = std::make_unique<lish::Args>(ctx.line_buffer_, 255);  // max for uint8_t
    uint8_t offset = 0;
    ctx.last_terminator = ctx.args->parse(offset);
}

WHEN(when_parse_second_slice, "I parse the second command slice") {
    auto& ctx = cuke::context<ArgsContext>();

    // Parse to get to the second command segment
    ctx.args = std::make_unique<lish::Args>(ctx.line_buffer_, 255);  // max for uint8_t
    uint8_t offset = 0;
    auto term1 = ctx.args->parse(offset);  // Parse first command

    // If there's a second command, parse it
    if (term1 != lish::Args::Terminator::EndOfLine) {
        ctx.args->parse(offset);  // Parse second command
    }
}

THEN(then_command_is, "the command should be {string}") {
    auto& ctx = cuke::context<ArgsContext>();
    std::string expected = CUKE_ARG(1);
    cuke::equal(std::string(ctx.args->command()), expected);
}

THEN(then_argument_count, "the argument count should be {int}") {
    auto& ctx = cuke::context<ArgsContext>();
    uint8_t expected = CUKE_ARG(1);
    cuke::equal(ctx.args->count(), expected);
}

THEN(then_argument_value, "argument {int} should be {string}") {
    auto& ctx = cuke::context<ArgsContext>();
    uint8_t index = CUKE_ARG(1);
    std::string expected = CUKE_ARG(2);
    const char* arg = (*ctx.args)[index];
    std::string actual = arg != nullptr ? std::string(arg) : std::string("");
    cuke::equal(actual, expected);
}

THEN(then_argument_is_null, "argument {int} should be null") {
    auto& ctx = cuke::context<ArgsContext>();
    uint8_t index = CUKE_ARG(1);
    const char* arg = (*ctx.args)[index];
    cuke::equal(arg == nullptr ? std::string("null") : std::string("not_null"), std::string("null"));
}

THEN(then_arguments_range_null, "arguments {int} to {int} return null") {
    auto& ctx = cuke::context<ArgsContext>();
    uint8_t start = CUKE_ARG(1);
    uint8_t end = CUKE_ARG(2);
    for (uint8_t i = start; i <= end; ++i) {
        const char* arg = (*ctx.args)[i];
        cuke::equal(arg == nullptr ? std::string("null") : std::string("not_null"), std::string("null"));
    }
}

/// @brief Create an Args instance with a nullptr buffer.
GIVEN(given_nullptr_buffer, "I have a nullptr buffer") {
    auto& ctx = cuke::context<ArgsContext>();
    // Create Args with nullptr - this tests the nullptr check in parse()
    ctx.args = std::make_unique<lish::Args>(nullptr, 255);
}

/// @brief Fill buffer with command string that reaches max_length with no null terminator.
/// @details This tests the defensive case where buffer isn't null-terminated at the end.
/// Fills buffer with "echo hello" followed by spaces, with no null terminator at the end.
GIVEN(given_buffer_no_null_terminator, "I have a command string that fills the buffer with no null terminator") {
    auto& ctx = cuke::context<ArgsContext>();
    std::string cmd = "echo hello";

    // Copy "echo hello" into buffer
    for (size_t i = 0; i < cmd.length(); ++i) {
        ctx.line_buffer_[i] = cmd[i];
    }

    // Fill the rest with spaces (won't create additional tokens)
    for (size_t i = cmd.length(); i < ArgsContext::MaxLineLength; ++i) {
        ctx.line_buffer_[i] = ' ';
    }

    // Do NOT null-terminate - this tests reaching max_length without \0
    // Create Args with max_length set to actual buffer size, so parse() reaches max_length
    ctx.args = std::make_unique<lish::Args>(ctx.line_buffer_, static_cast<uint8_t>(ArgsContext::MaxLineLength));
}

/// @brief Parse with slice_offset beyond the buffer's max length.
WHEN(when_parse_with_offset_beyond_length, "I parse with slice_offset beyond buffer length") {
    auto& ctx = cuke::context<ArgsContext>();
    // Create a fresh Args with small max_length (10 bytes)
    ctx.args = std::make_unique<lish::Args>(ctx.line_buffer_, 10);
    // Set slice_offset beyond max_length
    uint8_t offset = 255;  // Way beyond the 10-byte buffer
    ctx.last_terminator = ctx.args->parse(offset);
}

/// @brief Verify the terminator result of parse().
THEN(then_terminator_is, "the terminator should be {word}") {
    auto& ctx = cuke::context<ArgsContext>();
    std::string expected = CUKE_ARG(1);

    if (expected == "EndOfLine") {
        cuke::equal(ctx.last_terminator == lish::Args::Terminator::EndOfLine ? std::string("EndOfLine") : std::string("Other"), std::string("EndOfLine"));
    } else if (expected == "Semicolon") {
        cuke::equal(ctx.last_terminator == lish::Args::Terminator::Semicolon ? std::string("Semicolon") : std::string("Other"), std::string("Semicolon"));
    } else if (expected == "LogicalAnd") {
        cuke::equal(ctx.last_terminator == lish::Args::Terminator::LogicalAnd ? std::string("LogicalAnd") : std::string("Other"), std::string("LogicalAnd"));
    }
}

