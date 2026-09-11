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
  cmd_mode.cpp - Permission mode command implementation
*/

#include "app_context.h"
#include "cmd_mode.h"
#include <lish.h>
#include <stdio.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_MODE_NAME[] LISH_PROGMEM = "mode";
LISH_FLASH_STORAGE char CMD_MODE_HELP[] LISH_PROGMEM = "Get or set permission mode (0-6)";
LISH_FLASH_STORAGE char CMD_MODE_CURRENT[] LISH_PROGMEM = "Current mode: ";
LISH_FLASH_STORAGE char CMD_MODE_SET[] LISH_PROGMEM = "Mode set to: ";
LISH_FLASH_STORAGE char CMD_MODE_ERROR[] LISH_PROGMEM = "Error: mode must be 0-6";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_mode(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(ctx);
  if (args.count() == 0) {
    // Convert bitflag to mode number (0-6) using builtin_ctz (count trailing zeros)
    const uint8_t mode_num = __builtin_ctz(static_cast<uint8_t>(shell.mode()));
    char modeStr[4]{};
    snprintf(modeStr, 4, "%u", mode_num);
    lish::write_flash_string(CMD_MODE_CURRENT, shell);
    lish::write_string(modeStr, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return 0;
  }

  const char* mode_str = args[0];
  if (mode_str[0] >= '0' && mode_str[0] <= '6' && mode_str[1] == '\0') {
    uint8_t mode_num = mode_str[0] - '0';
    lish::CmdPermission new_mode = static_cast<lish::CmdPermission>(1 << mode_num);
    shell.set_mode(new_mode);
    lish::write_flash_string(CMD_MODE_SET, shell);
    shell.write('0' + mode_num);
    lish::write_flash_string(lish::NEWLINE, shell);
    return 0;
  }

  lish::write_flash_string(CMD_MODE_ERROR, shell);
  lish::write_flash_string(lish::NEWLINE, shell);
  return -1;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_mode_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_mode>(
    CMD_MODE_NAME, lish::CmdPermission::AllModes & ~lish::CmdPermission::Mode0, CMD_MODE_HELP);
