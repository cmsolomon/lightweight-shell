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

#ifndef LISH_LINE_BUFFER_H
#define LISH_LINE_BUFFER_H

#include <ctype.h>
#include <stdint.h>
#include <string.h>

namespace lish {

/// @file lishLineBuffer.h
/// @brief Line buffer structure for interactive command-line editing.
///
/// @details
/// LineBuffer groups the command-line buffer, cursor position, and line length
/// into a single struct. This reduces the number of references needed when passing
/// the buffer state to editing helpers (e.g., LineEditor), reducing register/stack
/// pressure on constrained systems like AVR (which has no data cache, so cache
/// locality isn't a factor here - the benefit is purely fewer bytes of pointer
/// per call).
///
/// Instead of passing three separate uint8_t* references (6 bytes on AVR, 12 bytes
/// on ARM Cortex-M4 - pointers are 2 and 4 bytes respectively), LineBuffer is passed
/// as a single struct reference (2 bytes on AVR, 4 bytes on ARM).

/// @brief Aggregates command-line buffer, cursor, and length into one unit.
///
/// @details
/// LineBuffer contains:
/// - A fixed-size character array (the command line).
/// - The current cursor position (where the next character will be inserted).
/// - The current line length (number of characters excluding '\0').
///
/// The Shell owns a LineBuffer and uses it to maintain the interactive command line.
/// Other components (LineEditor, auto-complete, etc.) operate on the LineBuffer via
/// non-owning references.
///
/// @tparam MaxLineLength The size of the buffer array (includes space for '\0').
///                      Typically 64 or more for shell commands.
///
/// ## Invariants
/// - **Null-termination**: `buffer[line_len] == '\0'` always.
/// - **Cursor bounds**: `cursor_pos ∈ [0, line_len]` (cursor may be at end-of-line).
/// - **Buffer capacity**: `line_len < MaxLineLength` (reserve 1 byte for '\0').
///
/// ## Example
/// @code
/// lish::LineBuffer<64> cmd_line;  // Create a 64-byte line buffer
/// cmd_line.line_len = 0;
/// cmd_line.cursor_pos = 0;
/// cmd_line.buffer[0] = '\0';
/// // Now ready for use with LineEditor or other editing helpers
/// @endcode
template <uint8_t MaxLineLength>
struct LineBuffer {
  /// The command-line text buffer (fixed-size array).
  /// Must be kept null-terminated: `buffer[line_len] == '\0'`.
  char buffer[MaxLineLength];

  /// Current length of the command line (characters, excluding '\0').
  /// Range: [0, MaxLineLength - 1].
  /// The null terminator is stored at buffer[line_len].
  uint8_t line_len;

  /// Current cursor position (0-indexed from start of buffer).
  /// Range: [0, line_len].
  /// Position 0 = start of line, line_len = end of line (past last character).
  uint8_t cursor_pos;

  /// @brief Default constructor; initializes buffer to empty state.
  ///
  /// Sets line_len and cursor_pos to 0, and writes a null terminator at buffer[0].
  /// The buffer is ready for use immediately after construction.
  LineBuffer() : line_len(0), cursor_pos(0) {
    buffer[0] = '\0';
  }

  /// @brief Checks if the buffer is blank (empty or contains only whitespace).
  /// @return True if line_len is 0 or all characters are whitespace, false otherwise.
  bool is_blank() const {
    if (line_len == 0) {
      return true;
    }
    for (uint8_t i = 0; i < line_len; ++i) {
      if (!isspace(static_cast<unsigned char>(buffer[i]))) {
        return false;
      }
    }
    return true;
  }

  /// @brief Resets the buffer to empty state.
  ///
  /// Clears line_len and cursor_pos to 0, and writes a null terminator at buffer[0].
  /// Used both when returning to the prompt after a command execution (Shell::on_submit())
  /// and when history browsing reaches its end and exits back to an empty line
  /// (InputProcessor::handle_history_next()).
  void clear() {
    line_len = 0;
    cursor_pos = 0;
    buffer[0] = '\0';
  }

  /// @brief Copies another same-size buffer's contents and positions cursor at end.
  ///
  /// @details
  /// Copies the entire `source` array, then uses `strnlen` to find the actual string
  /// length and position the cursor at the end.
  ///
  /// The source buffer size is enforced by type: source must be an array reference
  /// of exactly MaxLineLength bytes, so the entire array can always be safely read in
  /// one `memcpy`. This makes it suitable for copying between two LineBuffers of the
  /// same MaxLineLength (or any other source guaranteed to be a real MaxLineLength-byte
  /// array) - but NOT for copying from a variable-length or differently-sized source,
  /// such as a History entry (History::current() returns a plain, unsized `const char*`
  /// into a differently-sized buffer; blindly memcpy-ing MaxLineLength bytes from it
  /// could read past its actual data). InputProcessor::exit_browsing_and_copy() needs
  /// exactly that, which is why it does its own bounded strnlen+memcpy instead of using
  /// this method - this method has no current caller in the library itself.
  ///
  /// @param source Const reference to a char array of size MaxLineLength,
  ///               null-terminated within the first MaxLineLength-1 positions.
  ///
  /// ## Preconditions and Postconditions
  /// - **source size**: Exactly MaxLineLength (enforced by array reference type).
  /// - **source null-termination**: Must be null-terminated within MaxLineLength-1
  ///   (a precondition on the caller - not enforced by the type system).
  /// - **After operation**: buffer contains exact copy of source; line_len and
  ///   cursor_pos are set based on string length.
  ///
  /// @post line_len is set to the string length found by strnlen (max MaxLineLength-1).
  /// @post cursor_pos is set to line_len (end of line).
  /// @post buffer contains exact copy of source buffer.
  ///
  /// ## Example
  /// @code
  /// LineBuffer<64> src_line;
  /// LineBuffer<64> dst_line;
  /// dst_line.copy_from(src_line.buffer);  // Pass array reference
  /// // dst_line is now an exact copy of src_line
  /// // dst_line.line_len = length of the string in src_line
  /// // dst_line.cursor_pos = dst_line.line_len  (at end of line)
  /// @endcode
  void copy_from(const char(&source)[MaxLineLength]) {
    // Copy entire buffer (source is guaranteed to be MaxLineLength by type)
    memcpy(buffer, source, MaxLineLength);

    // Find actual string length and position cursor at end
    line_len = strnlen(source, MaxLineLength - 1);
    cursor_pos = line_len;
  }
};

}  // namespace lish

#endif  // LISH_LINE_BUFFER_H
