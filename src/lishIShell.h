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

#ifndef LISH_ISHELL_H
#define LISH_ISHELL_H

#include "lishCmdPermission.h"
#include "lishResult.h"
#include <stdint.h>

namespace lish {

/// @brief Abstract interface for shell implementations.
///
/// @details
/// Defines the public interface for shell operations independent of I/O adapter,
/// history implementation, or context type. Commands receive their context as
/// a void* parameter rather than through the shell interface.
///
/// ## Invariants
/// - IShell holds no state of its own - it is a pure interface. All state (current mode,
///   last result code, I/O, history, etc.) belongs to the concrete implementation (Shell).
/// - `mode` (as passed to set_mode() and returned by mode()) must always be a single active
///   mode bit (e.g. Mode0, Mode1), never a combined mask (e.g. Mode0 | Mode1) - the same
///   precondition documented on is_available()/is_hidden() in lishCmdPermission.h, which
///   the concrete implementation's permission checks rely on.
class IShell {
public:
  virtual ~IShell() = default;

  /// @brief Sets the active shell permission mode.
  ///
  /// @param mode The permission mode to activate for command gating. Must be a single
  ///             active mode bit (see class-level Invariants) - passing a combined mask
  ///             is not supported and will produce inconsistent permission checks.
  virtual void set_mode(CmdPermission mode) = 0;

  /// @brief Gets the current shell permission mode.
  ///
  /// @return The active CmdPermission mode.
  virtual CmdPermission mode() const = 0;

  /// @brief Services available input bytes and drives the interactive shell loop.
  ///
  /// @details
  /// Drains and processes every byte currently available from the I/O adapter, then
  /// returns - it does not block waiting for more input. Intended to be called
  /// repeatedly from the application's own polling loop (e.g. Arduino's loop()), not
  /// invoked once. Each processed byte is routed through the interactive line editor;
  /// a complete line triggers dispatch and status output the same way run_line() does,
  /// but through the interactive path (see InputProcessor).
  virtual void service() = 0;

  /// @brief Non-interactively executes a complete command line.
  ///
  /// @details
  /// Unlike service(), this dispatches a single line directly without going through the
  /// interactive line editor and without printing status output. Safe to call reentrantly
  /// from within a command handler that already has access to this IShell - e.g. to
  /// re-dispatch a reconstructed command line (see cmd_sudo.cpp in examples/BasicShell).
  /// The outermost call's result is what get_last_result_code() reflects once dispatch
  /// fully unwinds, regardless of any nested run_line() calls along the way.
  ///
  /// @param line Null-terminated command line in caller's buffer.
  /// @param length Number of valid bytes in the line.
  /// @return A CommandResult with the dispatch status and the command's own return code.
  virtual CommandResult run_line(char* line, uint8_t length) = 0;

  /// @brief Gets the exit code from the last executed command.
  ///
  /// @return CommandResult::code from the last dispatch. Returns 0 if no command has been
  ///         executed yet.
  virtual int8_t get_last_result_code() const = 0;

  /// @brief Writes a character to the shell's I/O adapter.
  ///
  /// @details
  /// Delegates to the underlying IOAdapter, making IShell duck-typed compatible
  /// with the write_string() and write_flash_string() helper functions in lishPlatform.h.
  /// This allows commands to output to any communication channel (UART, Bluetooth, USB, etc.)
  /// without needing specialized versions.
  ///
  /// @param c The character to write.
  virtual void write(char c) = 0;

  /// @brief Reads a character from the shell's I/O adapter with optional timeout.
  ///
  /// @details
  /// Delegates to the underlying IOAdapter, making IShell duck-typed compatible
  /// with code that needs to read input. Useful for interactive commands that need
  /// to prompt the user for input.
  ///
  /// @param c Reference to a char where the read character will be stored.
  /// @param timeout_ms Timeout in milliseconds:
  ///   - 0 (default): non-blocking, return false if no input available
  ///   - 0xFFFF: block forever until character arrives
  ///   - N: block for up to N milliseconds
  /// @return true if a character was successfully read, false on timeout or no input.
  virtual bool read(char& c, uint16_t timeout_ms = 0) = 0;

  /// @brief Prints the shell's prompt (custom or default).
  ///
  /// @details
  /// Called by InputProcessor during line redraws (tab completion, history navigation).
  /// Uses the custom prompt callback if one was provided, otherwise prints the default prompt.
  virtual void print_prompt() = 0;
};

}  // namespace lish

#endif  // LISH_ISHELL_H
