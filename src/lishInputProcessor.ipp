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

// Template implementations for InputProcessor class - included by lishInputProcessor.h
// DO NOT COMPILE THIS FILE DIRECTLY

namespace lish {

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
bool InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_ansi_or_special(const char c) {
  if (c == '\x1B') {
    ansi_state_ = AnsiState::Escape;
    return true;
  }

  if (ansi_state_ == AnsiState::Escape) {
    if (c == '[') {
      ansi_state_ = AnsiState::Csi;
      return true;
    } else if (c == 'O') {
      ansi_state_ = AnsiState::Ss3;
      return true;
    } else {
      ansi_state_ = AnsiState::Idle;
      return false;  // Unrecognized escape; let the character be processed normally
    }
  }

  if (ansi_state_ == AnsiState::Csi) {
    if (c >= '0' && c <= '9') {
      ansi_param_ = c;
    } else if (c == '~') {
      if (ansi_param_ == '1' || ansi_param_ == '7') {
        handle_home();
      } else if (ansi_param_ == '4' || ansi_param_ == '8') {
        handle_end();
      } else if (ansi_param_ == '3') {
        handle_delete();
      }
      ansi_state_ = AnsiState::Idle;
      ansi_param_ = '\0';
    } else if (c == 'A') {
      handle_history_prev();
      ansi_state_ = AnsiState::Idle;
    } else if (c == 'B') {
      handle_history_next();
      ansi_state_ = AnsiState::Idle;
    } else if (c == 'C') {
      handle_right();
      ansi_state_ = AnsiState::Idle;
    } else if (c == 'D') {
      handle_left();
      ansi_state_ = AnsiState::Idle;
    } else {
      ansi_state_ = AnsiState::Idle;
      ansi_param_ = '\0';
    }
    return true;
  }

  if (ansi_state_ == AnsiState::Ss3) {
    if (c == 'A') {
      handle_history_prev();
    } else if (c == 'B') {
      handle_history_next();
    } else if (c == 'C') {
      handle_right();
    } else if (c == 'D') {
      handle_left();
    } else if (c == 'F') {
      handle_end();
    } else if (c == 'H') {
      handle_home();
    }
    ansi_state_ = AnsiState::Idle;
    return true;
  }

  if (c == '\b' || c == 0x7F) {
    handle_backspace();
    return true;
  }

  if (c == '\t') {
    handle_tab_complete();
    return true;
  }

  return false;
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_insert(const char c) {
  exit_browsing_and_copy();

  uint8_t old_len = line_buf_.line_len;
  uint8_t old_cursor = line_buf_.cursor_pos;
  editor_.insert(c);

  if (line_buf_.line_len == old_len) {
    return;  // Buffer full
  }

  // Redraw from position where character was inserted (cursor moved forward by editor)
  partial_redraw_from(old_cursor);
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_backspace() {
  exit_browsing_and_copy();

  uint8_t old_pos = line_buf_.cursor_pos;
  editor_.backspace();

  if (line_buf_.cursor_pos == old_pos) {
    return;  // No change (at start)
  }

  // Send backspace to terminal to move cursor left, then redraw from new position
  io_.write('\b');
  partial_redraw_from(line_buf_.cursor_pos);
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_delete() {
  exit_browsing_and_copy();

  uint8_t old_len = line_buf_.line_len;
  editor_.delete_at_cursor();

  if (line_buf_.line_len == old_len) {
    return;  // No change (at/past end)
  }

  // Redraw from cursor position (where deletion occurred)
  partial_redraw_from(line_buf_.cursor_pos);
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_left() {
  exit_browsing_and_copy();

  uint8_t old_pos = line_buf_.cursor_pos;
  editor_.move_left();

  if (line_buf_.cursor_pos < old_pos) {
    io_.write('\b');
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_right() {
  exit_browsing_and_copy();

  uint8_t old_pos = line_buf_.cursor_pos;
  editor_.move_right();

  if (line_buf_.cursor_pos > old_pos) {
    if (ansi_state_ == AnsiState::Csi) {
      io_.write('\x1B');
      io_.write('[');
      io_.write('C');
    } else if (ansi_state_ == AnsiState::Ss3) {
      io_.write('\x1B');
      io_.write('O');
      io_.write('C');
    }
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_home() {
  exit_browsing_and_copy();

  for (uint8_t i = 0; i < line_buf_.cursor_pos; ++i) {
    io_.write('\b');
  }
  editor_.move_home();
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_end() {
  exit_browsing_and_copy();

  for (uint8_t i = line_buf_.cursor_pos; i < line_buf_.line_len; ++i) {
    io_.write(line_buf_.buffer[i]);
  }
  editor_.move_end();
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_tab_complete() {
  exit_browsing_and_copy();

  if (tab_complete(line_buf_, cmds_, current_mode_, io_)) {
    clear_line_and_redraw();  // Redraw prompt and line buffer after completion
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_history_prev() {
  if (history_.prev()) {
    clear_line_and_redraw_history();
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::handle_history_next() {
  if (history_.next()) {
    clear_line_and_redraw_history();
  } else {
    line_buf_.clear();
    clear_line_and_redraw();
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::exit_browsing_and_copy() {
  if (history_.is_browsing()) {
    const char* entry = history_.current();
    uint8_t len = strnlen(entry, MaxLineLength - 1);
    memcpy(line_buf_.buffer, entry, len);
    line_buf_.buffer[len] = '\0';
    line_buf_.line_len = len;
    line_buf_.cursor_pos = len;
    history_.cancel_browsing();
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::clear_line_and_redraw() {
  write_flash_string(ANSI_CLEAR_LINE, io_);
  shell_.print_prompt();

  for (uint8_t i = 0; i < line_buf_.line_len; ++i) {
    io_.write(line_buf_.buffer[i]);
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::partial_redraw_from(const uint8_t start_pos) {
  // Redraw characters from start_pos to end of line
  for (uint8_t i = start_pos; i < line_buf_.line_len; ++i) {
    io_.write(line_buf_.buffer[i]);
  }

  // Clear to end of line to remove any characters that were deleted
  write_flash_string(ANSI_CLEAR_TO_EOL, io_);

  // Move cursor back to correct position if it's not at the end
  uint8_t spaces_to_move = line_buf_.line_len - line_buf_.cursor_pos;
  if (spaces_to_move > 0) {
    // Send ESC[nD to move cursor left by n positions
    write_flash_string(ANSI_CURSOR_LEFT, io_);
    char num_buf[4];
    snprintf(num_buf, sizeof(num_buf), "%u", spaces_to_move);
    for (char* p = num_buf; *p != '\0'; ++p) {
      io_.write(*p);
    }
    write_flash_string(ANSI_CURSOR_LEFT_SUFFIX, io_);
  }
}

template <
  typename IOAdapter,
  typename LineEditorType,
  typename HistoryType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void InputProcessor<IOAdapter, LineEditorType, HistoryType, PromptCallbackType, NumCommands, MaxLineLength>::clear_line_and_redraw_history() {
  const char* entry = history_.current();

  write_flash_string(ANSI_CLEAR_LINE, io_);
  shell_.print_prompt();

  for (uint8_t i = 0; i < MaxLineLength; ++i) {
    if (entry[i] == '\0') {
      break;
    }
    io_.write(entry[i]);
  }
}

}  // namespace lish
