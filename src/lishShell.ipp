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

// Template implementations for Shell class - included by lishShell.h
// DO NOT COMPILE THIS FILE DIRECTLY

namespace lish {

template <
  typename IOAdapter,
  typename LineBufferType,
  typename LineEditorType,
  typename HistoryType,
  typename ContextType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
CommandResult Shell<IOAdapter, LineBufferType, LineEditorType, HistoryType, ContextType, PromptCallbackType, NumCommands, MaxLineLength>::dispatch_chain(char* const buf, const uint8_t length) {
  Args view(buf, length);
  uint8_t offset = 0;
  CommandResult result(ShellStatus::Ok, 0);
  Args::Terminator terminator;

  do {
    terminator = view.parse(offset);

    // Halt the chain here; see dispatch_chain's doc comment (lishShell.h) for why
    // no attempt is made to resynchronize and continue with a later segment.
    if (terminator == Args::Terminator::ParseError) {
      write_flash_string(ERROR_INVALID_ARGUMENT_FORMAT, io_);
      write_flash_string(NEWLINE, io_);
      return CommandResult(ShellStatus::Ok, -1);
    }

    if (view.command()[0] != '\0') {
      result = invoke_slice(view);
    }

  } while (terminator != Args::Terminator::EndOfLine &&
           (terminator == Args::Terminator::Semicolon || result.succeeded()));

  return result;
}

template <
  typename IOAdapter,
  typename LineBufferType,
  typename LineEditorType,
  typename HistoryType,
  typename ContextType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
CommandResult Shell<IOAdapter, LineBufferType, LineEditorType, HistoryType, ContextType, PromptCallbackType, NumCommands, MaxLineLength>::invoke_slice(Args& view) {
  const char* cmd = view.command();

  uint32_t cmd_hash = hash_cmd(cmd);

  if (cmd_hash == hash_cmd("help") && string_matches(cmd, HELP_COMMAND_STR, true)) {
    uint8_t code = execute_help(view, cmds_, current_mode_, io_);
    return CommandResult(ShellStatus::Ok, code);
  }

  for (size_t i = 0; i < NumCommands; ++i) {
    const CmdDescriptor* desc = read_flash<const CmdDescriptor*>(&cmds_[i]);
    if (!desc) {
      continue;
    }

    if (desc->matches_hash(cmd_hash) && desc->matches_name(cmd)) {
      CmdPermission perm = desc->permissions();

      if (is_hidden(perm, current_mode_)) {
        continue;
      }

      if (!is_available(perm, current_mode_)) {
        return CommandResult(ShellStatus::PermissionDenied, 0);
      }

      // Context passed directly as void* (erased trampoline does the casting)
      int8_t code = desc->execute(view, *this, ctx_);
      return CommandResult(ShellStatus::Ok, code);
    }
  }

  return CommandResult(ShellStatus::CommandNotFound, 0);
}

template <
  typename IOAdapter,
  typename LineBufferType,
  typename LineEditorType,
  typename HistoryType,
  typename ContextType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void Shell<IOAdapter, LineBufferType, LineEditorType, HistoryType, ContextType, PromptCallbackType, NumCommands, MaxLineLength>::print_status(const CommandResult result) {
  write_flash_string(NEWLINE, io_);

  if (result.status == ShellStatus::Ok && result.code == 0) {
    write_flash_string(STATUS_OK, io_);
  } else if (result.status == ShellStatus::Ok && result.code != 0) {
    write_flash_string(STATUS_ERROR_PREFIX, io_);
    write_flash_string(ERROR_CODE_PREFIX, io_);
    if (result.code < 0) {
      io_.write('-');
      int8_t abs_code = -result.code;
      io_.write('0' + (abs_code / 100) % 10);
      io_.write('0' + (abs_code / 10) % 10);
      io_.write('0' + (abs_code % 10));
    } else {
      io_.write('0' + (result.code / 100) % 10);
      io_.write('0' + (result.code / 10) % 10);
      io_.write('0' + (result.code % 10));
    }
    write_flash_string(STATUS_ERROR_SUFFIX, io_);
  } else {
    // The only two statuses that can reach here: dispatch_chain() (this function's only
    // caller, via on_submit()) never produces ShellStatus::EmptyCommand - that's exclusive
    // to run_line(), which never calls print_status() at all.
    write_flash_string(STATUS_ERROR_PREFIX, io_);
    if (result.status == ShellStatus::CommandNotFound) {
      write_flash_string(ERROR_COMMAND_NOT_FOUND, io_);
    } else {
      write_flash_string(ERROR_PERMISSION_DENIED, io_);
    }
    write_flash_string(STATUS_ERROR_SUFFIX, io_);
  }

  write_flash_string(NEWLINE, io_);
}

template <
  typename IOAdapter,
  typename LineBufferType,
  typename LineEditorType,
  typename HistoryType,
  typename ContextType,
  typename PromptCallbackType,
  size_t NumCommands,
  uint8_t MaxLineLength
>
void Shell<IOAdapter, LineBufferType, LineEditorType, HistoryType, ContextType, PromptCallbackType, NumCommands, MaxLineLength>::on_submit() {
  write_flash_string(NEWLINE,io_);

  if (!line_buf_.is_blank()) {
    CommandResult result = dispatch_chain(line_buf_.buffer, line_buf_.line_len);
    last_result_code_ = result.code;
    print_status(result);
  }

  line_buf_.clear();
  print_prompt();
}

}  // namespace lish