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

#ifndef LISH_HELP_H
#define LISH_HELP_H

#include "lishDefs.h"
#include "lishCmdPermission.h"
#include "lishCmdDescriptor.h"
#include "lishArgs.h"
#include <stdint.h>
#include <string.h>

namespace lish {

/// @file lishHelp.h
/// @brief Built-in help command for displaying available commands and their descriptions.
///
/// @details
/// Provides a built-in help command that doesn't need to be registered in the command array.
/// The help command lists all available commands or displays help text for a specific command,
/// respecting the current permission mode and visibility flags.
///
/// ## Behavior
/// - **No parameter**: Lists all command names in the command descriptor array. Hidden commands
///   are completely omitted. Unavailable but not hidden commands are listed with ANSI dim text.
///   Note: "help" itself is not in the descriptor array (see HELP_COMMAND_STR) and is not
///   included in this listing.
/// - **With command name parameter**: Displays the command name and its help text (if available).
///   Returns error if command not found or hidden in current mode. As with the no-parameter
///   listing, a command that is unavailable but not hidden is still shown (dimmed via ANSI_DIM)
///   rather than treated as an error. Special case: "help" is recognized here even though it has
///   no descriptor, so `help help` shows HELP_COMMAND_HELP instead of reporting itself as not
///   found.
///
/// ## Return Values
/// - **0**: Success (list displayed or command help found)
/// - **-1**: Command not found, hidden in current permission mode, or the argument itself was
///   empty/null (e.g. `help ""`)
/// - **-2**: Invalid argument count (more than one argument provided)
///
/// ## Invariants
/// - `matches_hash()` is a fast pre-filter only, never sufficient on its own: it must always be
///   followed by `matches_name()` before treating a descriptor as the real match. Two distinct
///   command names can share an FNV-1a hash (a genuine collision exists between "glbvs" and
///   "yacxa", both hashing to 2713492047 - see shell.feature's hash-collision scenarios); skipping
///   the name check would silently return the wrong command's help text.
/// - `current_mode` must represent exactly one active mode (a single set bit), matching the
///   precondition documented on `is_available()`/`is_hidden()` in lishCmdPermission.h - `hidden()`
///   and `available()` forward directly to those functions.
/// - Entries in `cmd_ptrs` may be `nullptr` (an unregistered slot) and are skipped; a non-null
///   entry must point to a valid, Flash-resident `CmdDescriptor`.
/// - No real command should be registered with the name "help": it is deliberately excluded from
///   `cmd_ptrs` and handled entirely by the special case below, so a descriptor literally named
///   "help" would never be reachable through the normal search loop.

/// Help text for the built-in help command itself, shown by `help help`. Needed
/// because help has no CmdDescriptor of its own (see HELP_COMMAND_STR in
/// lishDefs.h) - without this, execute_help() would report "help" as
/// command-not-found when asked for help about itself, since it never appears
/// in the descriptor array it searches. Local to this file (unlike
/// HELP_COMMAND_STR) since nothing else needs it.
LISH_FLASH_STORAGE char HELP_COMMAND_HELP[] LISH_PROGMEM = "Show available commands, or help for one command";

/// @brief Executes the built-in help command.
///
/// @details
/// Command tables are stored as arrays of pointers (in PROGMEM), where each pointer
/// references a descriptor also in PROGMEM - this avoids copying descriptors into RAM.
///
/// @tparam NumCommands Number of pointers in the descriptor pointer array.
/// @tparam IOAdapter I/O adapter type for writing output.
///
/// @param args Parsed command-line arguments.
/// @param cmd_ptrs Array of pointers to command descriptors (array itself is PROGMEM).
/// @param current_mode Current shell permission mode.
/// @param io I/O adapter for writing output.
///
/// @return int8_t - see the file-level "Return Values" section above.
template<size_t NumCommands, typename IOAdapter>
int8_t execute_help(
  const Args& args,
  const CmdDescriptor* const (&cmd_ptrs)[NumCommands],
  const CmdPermission current_mode,
  IOAdapter& io) {

  // No parameter: list all visible commands
  if (args.count() == 0) {
    for (size_t i = 0; i < NumCommands; ++i) {
      // Read pointer from PROGMEM array, then dereference to descriptor in PROGMEM
      const CmdDescriptor* ptr =
        read_flash<const CmdDescriptor*>(&cmd_ptrs[i]);
      if (!ptr) {
        continue;
      }

      // Check if hidden
      if (ptr->hidden(current_mode)) {
        continue;
      }

      if (!ptr->available(current_mode)) {
        write_flash_string(ANSI_DIM, io);
      }

      write_flash_string(ptr->name(), io);
      write_flash_string(ANSI_RESET, io);
      write_flash_string(NEWLINE, io);
    }

    return 0;
  }

  // One parameter: show help for specific command
  if (args.count() == 1) {
    const char* search_cmd = args[0];
    if (!search_cmd || search_cmd[0] == '\0') {
      return -1;
    }

    uint32_t search_hash = hash_cmd(search_cmd);

    // "help" has no CmdDescriptor of its own (see HELP_COMMAND_STR), so it would
    // never be found by the array search below. Special-case it here the same
    // way Shell::invoke_slice() and tab_complete() do, so `help help` works
    // instead of reporting itself as not found. Always shown (never dimmed or
    // hidden): the help command itself is unconditionally available in every
    // mode, with no CmdPermission of its own to check.
    if (search_hash == hash_cmd(HELP_COMMAND_STR) && string_matches(search_cmd, HELP_COMMAND_STR, true)) {
      write_flash_string(HELP_COMMAND_STR, io);
      write_flash_string(NEWLINE, io);
      write_flash_string(HELP_COMMAND_HELP, io);
      write_flash_string(NEWLINE, io);
      return 0;
    }

    for (size_t i = 0; i < NumCommands; ++i) {
      const CmdDescriptor* ptr =
        read_flash<const CmdDescriptor*>(&cmd_ptrs[i]);
      if (!ptr) {
        continue;
      }

      if (!ptr->matches_hash(search_hash)) {
        continue;
      }

      // Verify name match
      if (!ptr->matches_name(search_cmd)) {
        continue;
      }

      // Command found - check visibility
      if (ptr->hidden(current_mode)) {
        // found but hidden, so break and skip to 'not found'
        break;
      }

      if (!ptr->available(current_mode)) {
        write_flash_string(ANSI_DIM, io);
      }

      write_flash_string(ptr->name(), io);
      write_flash_string(ANSI_RESET, io);
      write_flash_string(NEWLINE, io);

      const char* help = ptr->help();
      if (help) {
        write_flash_string(help, io);
        write_flash_string(NEWLINE, io);
      }

      return 0;
    }

    write_flash_string(ERROR_COMMAND_NOT_FOUND, io);
    return -1;
  }

  write_flash_string(INVALID_ARGUMENTS, io);
  return -2;
}

}  // namespace lish

#endif  // LISH_HELP_H
