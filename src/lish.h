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

#ifndef LISH_H
#define LISH_H

#include "lishDefs.h"
#include "lishCmdPermission.h"
#include "lishCmdDescriptor.h"
#include "lishArgs.h"
#include "lishResult.h"
#include "lishIShell.h"
#include "lishLineBuffer.h"
#include "lishLineEditor.h"
#include "lishHistory.h"
#include "lishShell.h"

namespace lish {

/// @brief Factory function for creating a Shell with array-of-pointers to descriptors.
///
/// @details
/// Creates a Shell instance that uses an array of pointers to command descriptors.
/// Both the pointer array and the descriptors themselves are stored in PROGMEM (Flash),
/// ensuring complete PROGMEM coverage of command metadata and minimal RAM usage.
///
/// Reduces boilerplate by automatically instantiating LineBuffer, LineEditor, History,
/// and IOAdapter with the specified configuration, and deducing the number of commands
/// from the pointer array size.
///
/// History uses TypicalLineLength to size its buffer (typically 8-16 bytes for average
/// commands), while MaxLineLength supports longer chained commands. This separation
/// minimizes RAM usage by only allocating typical-size slots in history.
///
/// ## Usage Example
/// @code
/// // In cmd_echo.cpp:
/// LISH_FLASH_STORAGE char CMD_ECHO_NAME[] LISH_PROGMEM = "echo";
/// LISH_FLASH_STORAGE char CMD_ECHO_HELP[] LISH_PROGMEM = "Echo arguments back to output";
/// int8_t cmd_echo(const lish::Args& args, lish::IShell& shell, AppContext& ctx) { ... }
/// LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_echo_descriptor =
///   lish::CmdDescriptor::make<AppContext, &cmd_echo>(
///     CMD_ECHO_NAME, lish::CmdPermission::AllModes, CMD_ECHO_HELP);
///
/// // In BasicShell.ino:
/// LISH_FLASH_STORAGE lish::CmdDescriptor* const LISH_PROGMEM commands[] = {
///   &cmd_echo_descriptor,
///   &cmd_mode_descriptor,
///   ...
/// };
///
/// // HistoryLength=5, MaxLineLength=32 (line buffer); TypicalLineLength defaults to 10
/// static auto shell = lish::make_shell<SerialIOAdapter, 5, 32>(app_ctx, commands);
///
/// void setup() { shell.print_prompt(); }
/// void loop() { shell.service(); }  // Non-blocking; call repeatedly from loop()
///
/// // An optional custom prompt callback can be passed as the third argument:
/// //   make_shell<SerialIOAdapter, 5, 32>(app_ctx, commands, &custom_prompt)
/// // See examples/BasicShell for a complete, runnable version of this pattern
/// // with multiple commands, permission modes, and a custom prompt.
/// @endcode
///
/// @tparam IOAdapter I/O adapter type (must implement write(char)->void and
///         read(char&, uint16_t timeout_ms = 0)->bool - see IShell::read() in
///         lishIShell.h for the required timeout semantics)
/// @tparam HistoryLength Number of history entries (default 16)
/// @tparam MaxLineLength Maximum command line length in buffer (default 80)
/// @tparam TypicalLineLength Typical command length for history sizing (default 10)
/// @tparam ContextType Application context type passed to commands (auto-deduced)
/// @tparam NumCommands Number of commands (auto-deduced from pointer array)
///
/// @param ctx Reference to application context
/// @param cmd_ptrs Array of pointers to command descriptors (array itself in PROGMEM)
/// @param prompt_cb Optional prompt callback (nullptr uses default)
/// @param initial_mode Initial permission mode (default Mode0)
///
/// @return Shell instance with all command metadata in PROGMEM
///
/// @note The returned Shell is an rvalue and should be moved into persistent storage
///       (e.g., `auto shell = make_shell(...)`). Do not rely on the temporary.
template <typename IOAdapter, uint8_t HistoryLength = 16, uint8_t MaxLineLength = 80,
          uint8_t TypicalLineLength = 10, typename ContextType, size_t NumCommands>
Shell<
  IOAdapter,
  LineBuffer<MaxLineLength>,
  LineEditor<MaxLineLength>,
  History<HistoryLength, TypicalLineLength>,
  ContextType,
  void (*)(IShell&, ContextType&),
  NumCommands,
  MaxLineLength
>
make_shell(
  ContextType& ctx,
  const CmdDescriptor* const (&cmd_ptrs)[NumCommands],
  void (*prompt_cb)(IShell&, ContextType&) = nullptr,
  CmdPermission initial_mode = CmdPermission::Mode0
) {
  using ShellType = Shell<
    IOAdapter,
    LineBuffer<MaxLineLength>,
    LineEditor<MaxLineLength>,
    History<HistoryLength, TypicalLineLength>,
    ContextType,
    void (*)(IShell&, ContextType&),
    NumCommands,
    MaxLineLength
  >;
  return ShellType(ctx, cmd_ptrs, prompt_cb, initial_mode);
}

}  // namespace lish

#endif  // LISH_H