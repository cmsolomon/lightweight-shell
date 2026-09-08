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
  cmd_echo.cpp - Echo command implementation
*/

#include "app_context.h"
#include "cmd_echo.h"
#include <lish.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_ECHO_NAME[] LISH_PROGMEM = "echo";
LISH_FLASH_STORAGE char CMD_ECHO_HELP[] LISH_PROGMEM = "Echo arguments back to output";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_echo(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(ctx);
  for (uint8_t i = 0; i < args.count(); ++i) {
    lish::write_string(args[i], shell);
    shell.write(' ');
  }
  lish::write_flash_string(lish::NEWLINE, shell);
  return 0;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

const lish::CmdDescriptor LISH_PROGMEM cmd_echo_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_echo>(
    CMD_ECHO_NAME, lish::CmdPermission::Mode6, CMD_ECHO_HELP);
