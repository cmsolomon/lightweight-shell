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

#ifndef LISH_LINE_EDITOR_H
#define LISH_LINE_EDITOR_H

#include "lishLineBuffer.h"
#include <stdint.h>
#include <string.h>

namespace lish {

/// @file lishLineEditor.h
/// @brief Interactive line editing helper for command-line input.
///
/// @details
/// LineEditor provides insert, delete, and cursor movement operations on a LineBuffer.
/// It operates on a non-owning reference to a LineBuffer, modifying it in-place without
/// copying. All edits are immediately visible to the Shell through the same reference.
///
/// ## Design & State Management
/// LineEditor is a stateless adapter—all state lives in the LineBuffer it references.
/// It enforces LineBuffer invariants on every operation: null-termination, cursor bounds,
/// and buffer capacity. This design eliminates synchronization concerns between editor
/// state and buffer state.
///
/// ## Invariants After Each Operation
/// - **Null-termination**: `line_buf.buffer[line_buf.line_len] == '\0'`
/// - **Cursor bounds**: `line_buf.cursor_pos ∈ [0, line_buf.line_len]`
/// - **Buffer capacity**: `line_buf.line_len < MaxLineLength`

/// @brief Lightweight editor for interactive command-line input on a LineBuffer.
///
/// @details
/// LineEditor provides high-level editing operations (insert, backspace, delete,
/// cursor movement) that operate on a LineBuffer in-place. The editor does not own
/// the buffer—the Shell owns the LineBuffer and passes a reference to LineEditor
/// for interactive editing operations.
///
/// All operations maintain the LineBuffer's invariants: the buffer remains
/// null-terminated, the cursor stays within valid bounds, and capacity is never exceeded.
///
/// @tparam MaxLineLength The size of the LineBuffer's char array. Must match the
///                      LineBuffer passed to the constructor - enforced at compile time,
///                      since the constructor's parameter type is LineBuffer<MaxLineLength>&,
///                      not a runtime caller obligation.
template <uint8_t MaxLineLength>
class LineEditor {
public:
  /// @brief Constructs a LineEditor bound to a LineBuffer.
  ///
  /// @param line_buf Non-owning reference to a LineBuffer<MaxLineLength> to edit.
  ///                 The LineBuffer's lifetime must exceed the LineEditor's lifetime.
  ///
  /// @pre line_buf must be a valid, initialized LineBuffer<MaxLineLength>.
  /// @pre The LineBuffer invariants must be satisfied (null-termination, cursor bounds).
  explicit LineEditor(LineBuffer<MaxLineLength>& line_buf)
    : line_buf_(line_buf)
  {}

  /// @brief Inserts a character at the cursor position, shifting text rightward.
  ///
  /// @details
  /// Inserts `c` at cursor_pos, shifting all characters from cursor_pos through
  /// end-of-line one position right. The cursor advances one position, and line_len
  /// increases by one. The line is always kept null-terminated.
  ///
  /// If insertion would exceed MaxLineLength-1, the operation is silently ignored
  /// (no partial insertion, no error raised).
  ///
  /// @param c Character to insert.
  ///
  /// ## Invariants
  /// - If operation succeeds: line_len and cursor_pos each increment by 1.
  /// - If operation fails (full buffer): line_len and cursor_pos unchanged.
  /// - Buffer always remains null-terminated at line_buf_.buffer[line_buf_.line_len].
  void insert(const char c) {
    if (line_buf_.line_len >= MaxLineLength - 1) {
      return;
    }

    for (uint8_t i = line_buf_.line_len; i > line_buf_.cursor_pos; --i) {
      line_buf_.buffer[i] = line_buf_.buffer[i - 1];
    }
    line_buf_.buffer[line_buf_.cursor_pos] = c;
    line_buf_.line_len++;
    line_buf_.cursor_pos++;
    line_buf_.buffer[line_buf_.line_len] = '\0';
  }

  /// @brief Deletes the character before the cursor (backspace behavior).
  ///
  /// @details
  /// Moves the cursor left by one position and removes the character immediately
  /// before the cursor's old position (i.e. the character now at the new cursor
  /// position). Text after the cursor shifts left to fill the gap. Does nothing if
  /// cursor is at position 0 (start of line) or if line is empty.
  ///
  /// ## Invariants
  /// - If operation succeeds: line_len decrements by 1, cursor_pos decrements by 1.
  /// - If operation fails (at start): line_len and cursor_pos unchanged.
  /// - Buffer always remains null-terminated at line_buf_.buffer[line_buf_.line_len].
  void backspace() {
    if (line_buf_.cursor_pos == 0 || line_buf_.line_len == 0) {
      return;
    }

    line_buf_.cursor_pos--;
    for (uint8_t i = line_buf_.cursor_pos; i < line_buf_.line_len; ++i) {
      line_buf_.buffer[i] = line_buf_.buffer[i + 1];
    }
    line_buf_.line_len--;
  }

  /// @brief Deletes the character at the cursor position (forward delete).
  ///
  /// @details
  /// Removes the character at cursor_pos without moving the cursor. Text after the
  /// cursor shifts left to fill the gap. Does nothing if cursor is at or past
  /// end-of-line.
  ///
  /// ## Invariants
  /// - If operation succeeds: line_len decrements by 1, cursor_pos unchanged.
  /// - If operation fails (at/past end): line_len and cursor_pos unchanged.
  /// - Buffer always remains null-terminated at line_buf_.buffer[line_buf_.line_len].
  void delete_at_cursor() {
    if (line_buf_.cursor_pos >= line_buf_.line_len) {
      return;
    }

    for (uint8_t i = line_buf_.cursor_pos; i < line_buf_.line_len; ++i) {
      line_buf_.buffer[i] = line_buf_.buffer[i + 1];
    }
    line_buf_.line_len--;
  }

  /// @brief Moves the cursor one position left.
  ///
  /// @details
  /// Decrements cursor_pos by 1 if not already at position 0. Does nothing if cursor
  /// is at the start of the line.
  ///
  /// ## Invariants
  /// - Cursor remains in range [0, line_len].
  void move_left() {
    if (line_buf_.cursor_pos > 0) {
      line_buf_.cursor_pos--;
    }
  }

  /// @brief Moves the cursor one position right.
  ///
  /// @details
  /// Increments cursor_pos by 1 if not already at end-of-line (cursor_pos == line_len).
  /// Does nothing if cursor is at or past the end.
  ///
  /// ## Invariants
  /// - Cursor remains in range [0, line_len].
  void move_right() {
    if (line_buf_.cursor_pos < line_buf_.line_len) {
      line_buf_.cursor_pos++;
    }
  }

  /// @brief Moves the cursor to the start of the line (position 0).
  ///
  /// ## Invariants
  /// - Cursor is always <= line_len after this operation.
  void move_home() {
    line_buf_.cursor_pos = 0;
  }

  /// @brief Moves the cursor to the end of the line (position line_len).
  ///
  /// @details
  /// Positions the cursor one past the last character (at the null terminator).
  ///
  /// ## Invariants
  /// - Cursor remains within [0, line_len] after this operation.
  void move_end() {
    line_buf_.cursor_pos = line_buf_.line_len;
  }

  /// @brief Gets the current cursor position.
  ///
  /// @return Cursor position in range [0, line_len].
  ///         0 = start of line, line_len = end of line (past last character).
  uint8_t cursor_pos() const { return line_buf_.cursor_pos; }

private:
  LineBuffer<MaxLineLength>& line_buf_;  ///< Non-owning reference to the line buffer.
};

}  // namespace lish

#endif  // LISH_LINE_EDITOR_H
