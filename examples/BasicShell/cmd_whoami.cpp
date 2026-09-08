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
  cmd_whoami.cpp - Display current logged-in user
*/

#include "app_context.h"
#include "cmd_whoami.h"
#include <lish.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_WHOAMI_NAME[] LISH_PROGMEM = "whoami";
LISH_FLASH_STORAGE char CMD_WHOAMI_HELP[] LISH_PROGMEM = "Display current user";
LISH_FLASH_STORAGE char CMD_WHOAMI_NOT_LOGGED_IN[] LISH_PROGMEM = "not logged in";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_whoami(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  if (args.count() != 0) {
    lish::write_flash_string(lish::NEWLINE, shell);
    return -1;
  }

  if (ctx.current_user[0] != '\0') {
    lish::write_string(ctx.current_user, shell);
  } else {
    lish::write_flash_string(CMD_WHOAMI_NOT_LOGGED_IN, shell);
  }

  lish::write_flash_string(lish::NEWLINE, shell);
  return 0;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

const lish::CmdDescriptor LISH_PROGMEM cmd_whoami_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_whoami>(
    CMD_WHOAMI_NAME, lish::CmdPermission::AllModes, CMD_WHOAMI_HELP);
