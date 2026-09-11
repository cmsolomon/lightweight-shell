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

/*
  cmd_sudo.cpp - Elevated privilege command execution
*/

#include "app_context.h"
#include "cmd_sudo.h"
#include <lish.h>
#include <string.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_SUDO_NAME[] LISH_PROGMEM = "sudo";
LISH_FLASH_STORAGE char CMD_SUDO_HELP[] LISH_PROGMEM = "Execute command with elevated privileges (Mode 6)";
LISH_FLASH_STORAGE char CMD_SUDO_USAGE[] LISH_PROGMEM = "Usage: sudo <command> [args...]";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_sudo(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(ctx);

  if (args.count() == 0) {
    lish::write_flash_string(CMD_SUDO_USAGE, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return -1;
  }

  // Capture current mode to restore later
  lish::CmdPermission original_mode = shell.mode();

  // Reconstruct command line with spaces
  // Args buffer has nulls where spaces were; we convert them back

  // Find the start of the command to execute (first arg after "sudo")
  char* cmd_start = const_cast<char*>(args[0]);

  // Find the end of the command line (start of last argument)
  char* cmd_end = const_cast<char*>(args[args.count() - 1]);

  // Find the actual end (after the last argument's string)
  char* actual_end = cmd_end + strlen(cmd_end);

  // Convert all nulls back to spaces between cmd_start and actual_end
  // (except the final null terminator)
  for (char* addr = cmd_start; addr < actual_end; ++addr) {
    if (*addr == '\0') {
      *addr = ' ';
    }
  }

  // Calculate the length of the command to execute
  uint16_t cmd_len = actual_end - cmd_start;

  // Switch to elevated mode
  shell.set_mode(lish::CmdPermission::Mode6);

  // Execute the reconstructed command
  shell.run_line(cmd_start, cmd_len);

  // Restore original mode
  shell.set_mode(original_mode);

  return 0;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_sudo_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_sudo>(
    CMD_SUDO_NAME, lish::CmdPermission::Mode2, CMD_SUDO_HELP);
