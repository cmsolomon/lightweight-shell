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
  cmd_uptime.cpp - System uptime reporting
*/

#include "app_context.h"
#include "cmd_uptime.h"
#include <lish.h>
#include <stdio.h>
#include <Arduino.h>

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_UPTIME_NAME[] LISH_PROGMEM = "uptime";
LISH_FLASH_STORAGE char CMD_UPTIME_HELP[] LISH_PROGMEM = "Report system uptime";
LISH_FLASH_STORAGE char CMD_UPTIME_USAGE[] LISH_PROGMEM = "Usage: uptime (no arguments)";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_uptime(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(ctx);

  if (args.count() != 0) {
    lish::write_flash_string(CMD_UPTIME_USAGE, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return -1;
  }

  // Get uptime in milliseconds
  uint32_t total_ms = millis();

  // Calculate days, hours, minutes, seconds with milliseconds
  uint32_t days = total_ms / (24UL * 60 * 60 * 1000);
  uint32_t remainder = total_ms % (24UL * 60 * 60 * 1000);

  uint32_t hours = remainder / (60UL * 60 * 1000);
  remainder %= (60UL * 60 * 1000);

  uint32_t minutes = remainder / (60UL * 1000);
  uint32_t seconds_with_ms = remainder % (60UL * 1000);

  // Format and write uptime
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "Uptime: %lu d, %lu h, %lu m, %lu.%03lu s",
           days, hours, minutes, seconds_with_ms / 1000, seconds_with_ms % 1000);

  lish::write_string(buffer, shell);
  lish::write_flash_string(lish::NEWLINE, shell);

  return 0;
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_uptime_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_uptime>(
    CMD_UPTIME_NAME, lish::CmdPermission::AllModes, CMD_UPTIME_HELP);
