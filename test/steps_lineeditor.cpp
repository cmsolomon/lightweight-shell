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
// LINE EDITOR TESTS (lineeditor.feature)
// ============================================================================
/// Tests for the LineEditor class: insertion, deletion, cursor movement, and text manipulation.

struct LineEditorContext {
    static constexpr size_t MaxLineLength = 64;
    lish::LineBuffer<MaxLineLength> line_buf;
};

WHEN(when_create_line_editor, "I create a line editor") {
    auto& ctx = cuke::context<LineEditorContext>();
    ctx.line_buf.clear();
}

WHEN(when_create_line_editor_with_text, "I create a line editor with {string}") {
    std::string initial = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    ctx.line_buf.clear();

    // Create a temp buffer and copy the string into it
    char temp_array[LineEditorContext::MaxLineLength];
    std::memset(temp_array, 0, LineEditorContext::MaxLineLength);
    std::strncpy(temp_array, initial.c_str(), LineEditorContext::MaxLineLength - 1);

    ctx.line_buf.copy_from(temp_array);
}

WHEN(when_create_line_editor_with_max_length, "I create a line editor with maximum length {string}") {
    std::string initial = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    ctx.line_buf.clear();

    // Create a temp buffer and copy the string into it
    char temp_array[LineEditorContext::MaxLineLength];
    std::memset(temp_array, 0, LineEditorContext::MaxLineLength);
    std::strncpy(temp_array, initial.c_str(), LineEditorContext::MaxLineLength - 1);

    ctx.line_buf.copy_from(temp_array);
}

WHEN(when_insert_char, "I insert {string}") {
    std::string char_str = CUKE_ARG(1);
    char c = char_str[0];
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.insert(c);
}

WHEN(when_move_cursor_to, "I move cursor to position {int}") {
    uint8_t pos = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    // Move cursor to the desired position
    while (editor.cursor_pos() < pos) {
        editor.move_right();
    }
    while (editor.cursor_pos() > pos) {
        editor.move_left();
    }
}

WHEN(when_backspace, "I backspace") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.backspace();
}

WHEN(when_delete_at_cursor, "I delete at cursor") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.delete_at_cursor();
}

WHEN(when_move_cursor_left, "I move cursor left") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.move_left();
}

WHEN(when_move_cursor_right, "I move cursor right") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.move_right();
}

WHEN(when_move_cursor_home, "I move cursor home") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.move_home();
}

WHEN(when_move_cursor_end, "I move cursor end") {
    auto& ctx = cuke::context<LineEditorContext>();
    lish::LineEditor<LineEditorContext::MaxLineLength> editor(ctx.line_buf);
    editor.move_end();
}

THEN(then_line_editor_line_len_should_be, "line length should be {int}") {
    uint8_t expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.line_len), static_cast<int>(expected));
}

THEN(then_line_editor_cursor_pos_should_be, "cursor position should be {int}") {
    uint8_t expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.cursor_pos), static_cast<int>(expected));
}

THEN(then_buffer_content_should_be, "buffer content should be {string}") {
    std::string expected = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    std::string actual(ctx.line_buf.buffer, ctx.line_buf.line_len);
    cuke::equal(actual, expected);
}

THEN(then_buffer_should_be_null_terminated_at, "buffer should be null-terminated at position {int}") {
    uint8_t pos = CUKE_ARG(1);
    auto& ctx = cuke::context<LineEditorContext>();
    cuke::equal(ctx.line_buf.buffer[pos], '\0');
}

THEN(then_line_editor_cursor_pos_equal_line_len, "cursor position should equal line length") {
    auto& ctx = cuke::context<LineEditorContext>();
    cuke::equal(static_cast<int>(ctx.line_buf.cursor_pos), static_cast<int>(ctx.line_buf.line_len));
}

THEN(then_buffer_should_not_contain_at_end, "buffer should not contain {string} at end") {
    std::string char_str = CUKE_ARG(1);
    char c = char_str[0];
    auto& ctx = cuke::context<LineEditorContext>();
    if (ctx.line_buf.line_len > 0) {
        char last_char = ctx.line_buf.buffer[ctx.line_buf.line_len - 1];
        cuke::equal(last_char != c, true);
    }
}
