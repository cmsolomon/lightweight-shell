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

#ifndef LISH_INPUT_PROCESSOR_H
#define LISH_INPUT_PROCESSOR_H

#include "lishLineBuffer.h"
#include "lishLineEditor.h"
#include "lishCmdDescriptor.h"
#include "lishCmdPermission.h"
#include "lishTabComplete.h"
#include "lishDefs.h"
#include <stdio.h>
#include <stdint.h>

namespace lish {

/// @file lishInputProcessor.h
/// @brief Interactive input processing and line editing state machine.
///
/// @details
/// InputProcessor handles all character-level input processing for interactive shell mode:
/// - ANSI escape sequence parsing (cursor movement, function keys)
/// - Character insertion, deletion, and cursor navigation
/// - History browsing (up/down arrows)
/// - Tab completion with command lookup
/// - Screen redraw on edits
/// - Line ending detection (CR/LF with deduplication)
///
/// InputProcessor operates on a LineBuffer bound by reference at construction, delegating
/// editing to LineEditor and history navigation to the injected History type. When the user
/// presses Enter, process_char() returns true, signaling that the line is ready for command
/// execution. The Shell responds by calling on_submit(), which dispatches the line (via
/// dispatch_chain()), prints its status, and clears the buffer for the next line.
///
/// @tparam IOAdapter I/O adapter type for writing output (redraw, completion list).
/// @tparam HistoryType History manager type (must implement prev/next/current/push).
/// @tparam NumCommands Number of commands in the descriptor array.
/// @tparam MaxLineLength Maximum line buffer size.
///
/// ## Invariants
/// - `line_buf_` is a non-owning reference bound at construction (typically Shell's own
///   persistent LineBuffer) - InputProcessor never reassigns or reallocates it.
/// - While `history_.is_browsing()` is true, `line_buf_` is NOT updated to reflect the
///   browsed entry - the browsed content is written directly to `io_` by
///   clear_line_and_redraw_history(). `line_buf_` is only updated when browsing ends, via
///   exit_browsing_and_copy(), triggered by any edit or by pressing Enter.
/// - `ansi_param_` retains only the single most-recently-typed digit of a CSI numeric
///   parameter sequence (each digit overwrites the last, none are accumulated) - a
///   hypothetical two-digit code such as "24~" would be treated as if only "4" had been
///   typed. Harmless today since every recognized code ("1","3","4","7","8") is a single
///   digit, but a real limitation for extending the recognized code set.

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength = 64
>
class InputProcessor {
  enum class AnsiState : uint8_t {
    Idle,   ///< No escape sequence in progress
    Escape, ///< ESC received; awaiting CSI or SS3
    Csi,    ///< CSI (ESC[) sequence detected
    Ss3     ///< SS3 (ESC O) sequence detected
  };

public:
  /// @brief Constructs an InputProcessor with owned editing and history infrastructure.
  ///
  /// @param line_buf Non-owning reference to Shell's LineBuffer (data to be edited).
  /// @param io Non-owning reference to IOAdapter for output (redraw, completion).
  /// @param cmd_ptrs Array of pointers to command descriptors (for tab completion).
  /// @param current_mode Reference to Shell's current permission mode.
  /// @param shell Non-owning reference to IShell for printing prompts during redraw.
  InputProcessor(
    LineBuffer<MaxLineLength>& line_buf,
    IOAdapter& io,
    const CmdDescriptor* const (&cmd_ptrs)[NumCommands],
    CmdPermission& current_mode,
    IShell& shell
  )
    : line_buf_(line_buf),
      editor_(line_buf),
      io_(io),
      cmds_(cmd_ptrs),
      current_mode_(current_mode),
      shell_(shell),
      ansi_state_(AnsiState::Idle),
      ansi_param_('\0'),
      last_char_('\0')
  {
  }


  /// @brief Processes a single input character and updates the line buffer.
  ///
  /// @details
  /// Handles all character-level input: printable text, backspace, delete, cursor movement,
  /// history navigation, tab completion, and line submission. The LineBuffer is updated
  /// in-place via the owned LineEditor; output (redraw, completion list) is written to IOAdapter.
  ///
  /// @param c The character to process (any byte value).
  ///
  /// @return true if the user pressed Enter and the line is ready for command execution;
  ///         false if still editing the current line, or if `c` is the second character of
  ///         a CR/LF pair already consumed as the same line-ending event (see the file-level
  ///         "Line ending detection" note).
  ///
  /// @post If returns true: line_buf_ contains the ready-to-execute command line.
  /// @post If returns false: line_buf_ may be modified (insert/delete/cursor/history ops).
  bool process_char(const char c) {
    if (c == '\r' || c == '\n') {
      if (last_char_ == '\r' || last_char_ == '\n') {
        if ((last_char_ == '\r' && c == '\n') || (last_char_ == '\n' && c == '\r')) {
          last_char_ = '\0';
          return false;
        }
      }
      last_char_ = c;
      exit_browsing_and_copy();
      if (!line_buf_.is_blank()) {
        history_.push(line_buf_);
      }
      history_.cancel_browsing();
      return true;  // Line ready for execution
    }

    last_char_ = c;

    if (handle_ansi_or_special(c)) {
      return false;
    }

    if (c >= 0x20 && c < 0x7F) {
      handle_insert(c);
      return false;
    }

    return false;
  }

private:
  /// @brief Handles ANSI escape sequences and special characters (backspace, tab).
  /// @param c The character to process
  /// @return true if the character was handled as special/ANSI, false if normal printable
  bool handle_ansi_or_special(const char c);

  /// @brief Inserts a character at the cursor position and redraws the line.
  ///
  /// @details
  /// Exits history browsing mode if active, delegates insertion to LineEditor, and redraws
  /// from the insertion point to the end of the line (not the whole line) if the buffer
  /// wasn't full.
  ///
  /// @param c The character to insert (printable ASCII, 0x20-0x7E).
  void handle_insert(const char c);

  /// @brief Deletes the character before the cursor and redraws the line.
  ///
  /// @details
  /// Exits history browsing mode if active, delegates backspace to LineEditor, and redraws
  /// from the new cursor position to the end of the line (not the whole line) if deletion
  /// occurred. Does nothing if cursor is already at the start of the line.
  void handle_backspace();

  /// @brief Deletes the character at the cursor position and redraws the line.
  ///
  /// @details
  /// Exits history browsing mode if active, delegates delete to LineEditor, and redraws
  /// from the cursor position to the end of the line (not the whole line) if deletion
  /// occurred. Does nothing if cursor is at or past the end of the line.
  void handle_delete();

  /// @brief Moves the cursor one position left and outputs backspace if successful.
  ///
  /// @details
  /// Exits history browsing mode if active, delegates cursor movement to LineEditor,
  /// and writes a backspace character to the terminal if the cursor actually moved.
  void handle_left();

  /// @brief Moves the cursor one position right and outputs ANSI escape sequence if successful.
  ///
  /// @details
  /// Exits history browsing mode if active, delegates cursor movement to LineEditor, and (if
  /// the cursor moved) echoes back an arrow-right escape sequence matching whichever style
  /// triggered this call: ESC[C (CSI) or ESC O C (SS3).
  void handle_right();

  /// @brief Moves the cursor to the start of the line and outputs appropriate backspaces.
  ///
  /// @details
  /// Exits history browsing mode if active, outputs backspace characters to move the
  /// terminal cursor from its current position to the start of the line, then delegates
  /// the actual cursor movement to LineEditor.
  void handle_home();

  /// @brief Moves the cursor to the end of the line and outputs remaining characters.
  ///
  /// @details
  /// Exits history browsing mode if active, outputs the remaining line characters from the
  /// current cursor position to the end, then delegates the actual cursor movement to LineEditor.
  void handle_end();

  /// @brief Handles tab key for command-name auto-completion.
  ///
  /// @details
  /// Exits history browsing mode if active, invokes the tab_complete function to find
  /// and complete matching command names based on the current line buffer content.
  /// If completion succeeds (single match or multiple candidates listed), clears and
  /// redraws the prompt and line buffer to reflect the updated state.
  void handle_tab_complete();

  /// @brief Navigates to the previous entry in the command history.
  ///
  /// @details
  /// Attempts to move to the previous history entry. If successful (history_.prev() returns true),
  /// clears the current line display and redraws it with the history entry content without
  /// modifying the working line buffer. Does nothing if already at the oldest history entry.
  void handle_history_prev();

  /// @brief Navigates to the next entry in the command history.
  ///
  /// @details
  /// Attempts to move to the next history entry. If successful (history_.next() returns true),
  /// displays that entry. If we've reached the end of history (next() returns false),
  /// clears the working line buffer and redisplays an empty line, returning to the
  /// original editing context before any history navigation.
  void handle_history_next();

  /// @brief If browsing history, copy the current entry to line_buf and exit browsing.
  ///        Otherwise, does nothing (already editing line_buf).
  void exit_browsing_and_copy();

  /// @brief Clear line and redraw entire line from line_buf_.
  ///        Used for tab completion, and for exiting history browsing back to an empty
  ///        line - NOT for displaying a browsed history entry, which is drawn from
  ///        history_.current() by clear_line_and_redraw_history() instead, without ever
  ///        touching line_buf_.
  void clear_line_and_redraw();

  /// @brief Redraw line from specified position to end, then reposition cursor.
  ///        Optimized for edits (insert/delete) to reduce flicker.
  /// @param start_pos Position to start redrawing from (typically cursor position before the edit)
  void partial_redraw_from(const uint8_t start_pos);

  /// @brief Clear line and redraw from history_.current() (when browsing history).
  ///        Does NOT modify line_buf_.
  void clear_line_and_redraw_history();

  LineBuffer<MaxLineLength>& line_buf_;
  LineEditorType editor_;
  HistoryType history_;
  IOAdapter& io_;
  const CmdDescriptor* const (&cmds_)[NumCommands];
  CmdPermission& current_mode_;
  IShell& shell_;

  AnsiState ansi_state_;
  char ansi_param_;
  char last_char_;
};

}  // namespace lish

#include "lishInputProcessor.ipp"

#endif  // LISH_INPUT_PROCESSOR_H
