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
  cmd_logout.cpp - Logout and clear user session
*/

#include "app_context.h"
#include "cmd_logout.h"
#include <lish.h>
#include <string.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_LOGOUT_NAME[] LISH_PROGMEM = "logout";
LISH_FLASH_STORAGE char CMD_LOGOUT_HELP[] LISH_PROGMEM = "Logout and return to mode 0";
LISH_FLASH_STORAGE char CMD_LOGOUT_SUCCESS[] LISH_PROGMEM = "Logged out";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_logout(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(args);  // Ignore any arguments

  // Clear current user
  ctx.current_user[0] = '\0';

  // Reset to mode 0
  shell.set_mode(lish::CmdPermission::Mode0);

  lish::write_flash_string(CMD_LOGOUT_SUCCESS, shell);
  lish::write_flash_string(lish::NEWLINE, shell);

  return 0;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

// Available in all modes except Mode0, hidden in Mode0
LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_logout_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_logout>(
    CMD_LOGOUT_NAME,
    (lish::CmdPermission::AllModes & ~lish::CmdPermission::Mode0) | lish::CmdPermission::HideWhenUnavailable,
    CMD_LOGOUT_HELP);
