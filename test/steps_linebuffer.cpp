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
#include <string>

// ============================================================================
// LINE BUFFER TESTS (linebuffer.feature)
// ============================================================================
/// Tests for the LineBuffer class: capacity, null termination, and content management.

struct LineBufferContext {
    static constexpr size_t BufferSize = 64;
    lish::LineBuffer<BufferSize> line_buf;
};

WHEN(when_create_line_buffer, "I create a new line buffer") {
    // Context is created lazily, just access it to ensure it exists
    auto& ctx = cuke::context<LineBufferContext>();
    ctx.line_buf.clear();  // Ensure clean state
}

WHEN(when_copy_into_buffer, "I copy {string} into the buffer") {
    std::string source = CUKE_ARG(1);
    auto& ctx = cuke::context<LineBufferContext>();

    // For empty string, clear the buffer
    if (source.empty()) {
        ctx.line_buf.clear();
    } else {
        // Create a temp buffer of same size as our test buffer
        char temp_buffer[LineBufferContext::BufferSize];
        std::memset(temp_buffer, 0, LineBufferContext::BufferSize);
        std::strncpy(temp_buffer, source.c_str(), LineBufferContext::BufferSize - 1);
        temp_buffer[LineBufferContext::BufferSize - 1] = '\0';

        ctx.line_buf.copy_from(temp_buffer);
    }
}

THEN(then_line_length_should_be, "the line length should be {int}") {
    uint8_t expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.line_len), static_cast<int>(expected));
}

THEN(then_cursor_pos_should_be, "the cursor position should be {int}") {
    uint8_t expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.cursor_pos), static_cast<int>(expected));
}

THEN(then_buffer_contains_only_null, "the buffer should contain only null terminator") {
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.line_len), 0);
    cuke::equal(ctx.line_buf.buffer[0], '\0');
}

WHEN(when_clear_buffer, "I clear the buffer") {
    auto& ctx = cuke::context<LineBufferContext>();
    ctx.line_buf.clear();
}

THEN(then_buffer_contains_string, "the buffer should contain {string}") {
    std::string expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(std::string(ctx.line_buf.buffer, ctx.line_buf.line_len), expected);
}

THEN(then_cursor_equals_length, "the cursor position should equal the line length") {
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.cursor_pos),
                static_cast<int>(ctx.line_buf.line_len));
}

THEN(then_buffer_null_terminated_at, "the buffer should be null-terminated at position {int}") {
    uint8_t pos = CUKE_ARG(1);
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(ctx.line_buf.buffer[pos], '\0');
}

THEN(then_buffer_is_blank, "the buffer should be blank") {
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(ctx.line_buf.is_blank(), true);
}

THEN(then_buffer_is_not_blank, "the buffer should not be blank") {
    auto& ctx = cuke::context<LineBufferContext>();
    cuke::equal(ctx.line_buf.is_blank(), false);
}
