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
  cmd_login.cpp - User authentication and login
*/

#include "app_context.h"
#include "cmd_login.h"
#include <lish.h>
#include <string.h>
#include <ctype.h>

// Case-insensitive string comparison (portable alternative to strncasecmp)
static int case_insensitive_cmp(const char* const a, const char* const b, const uint8_t len) {
  for (uint8_t i = 0; i < len; ++i) {
    char ca = tolower((unsigned char)a[i]);
    char cb = tolower((unsigned char)b[i]);
    if (ca != cb) {
      return ca - cb;
    }
  }
  return 0;
}

// ============================================================================
// Localized PROGMEM Strings
// ============================================================================

LISH_FLASH_STORAGE char CMD_LOGIN_NAME[] LISH_PROGMEM = "login";
LISH_FLASH_STORAGE char CMD_LOGIN_HELP[] LISH_PROGMEM = "Login (users: unpriv/pass1, priv/pass2)";
LISH_FLASH_STORAGE char CMD_LOGIN_USAGE[] LISH_PROGMEM = "Usage: login (no arguments)";
LISH_FLASH_STORAGE char CMD_LOGIN_PROMPT_USER[] LISH_PROGMEM = "Username: ";
LISH_FLASH_STORAGE char CMD_LOGIN_PROMPT_PASS[] LISH_PROGMEM = "Password: ";
LISH_FLASH_STORAGE char CMD_LOGIN_SUCCESS[] LISH_PROGMEM = "Login successful";
LISH_FLASH_STORAGE char CMD_LOGIN_FAILED[] LISH_PROGMEM = "Login failed";

// ============================================================================
// Command Implementation
// ============================================================================

int8_t cmd_login(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  if (args.count() != 0) {
    lish::write_flash_string(CMD_LOGIN_USAGE, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return -1;
  }

  // Prompt for username (block forever for input)
  lish::write_flash_string(CMD_LOGIN_PROMPT_USER, shell);
  char username[16] = {0};
  uint8_t username_len = 0;
  char c;

  while (shell.read(c, 0xFFFF) && c != '\r' && c != '\n' && username_len < 15) {
    username[username_len++] = c;
    shell.write(c);
  }
  lish::write_flash_string(lish::NEWLINE, shell);

  // Prompt for password (block forever for input)
  lish::write_flash_string(CMD_LOGIN_PROMPT_PASS, shell);
  char password[16] = {0};
  uint8_t password_len = 0;

  while (shell.read(c, 0xFFFF) && c != '\r' && c != '\n' && password_len < 15) {
    password[password_len++] = c;
    shell.write('*');  // Echo asterisks instead of password
  }
  lish::write_flash_string(lish::NEWLINE, shell);

  // Validate credentials
  bool valid = false;
  uint8_t mode_offset = 0;

  if (username_len == 6 && case_insensitive_cmp(username, "unpriv", username_len) == 0 &&
      strncmp(password, "pass1", password_len) == 0 && password_len == 5) {
    valid = true;
    mode_offset = 1;  // Mode1
  } else if (username_len == 4 && case_insensitive_cmp(username, "priv", username_len) == 0 &&
             strncmp(password, "pass2", password_len) == 0 && password_len == 5) {
    valid = true;
    mode_offset = 2;  // Mode2
  }

  if (valid) {
    strncpy(ctx.current_user, username, sizeof(ctx.current_user) - 1);
    ctx.current_user[sizeof(ctx.current_user) - 1] = '\0';
    shell.set_mode(static_cast<lish::CmdPermission>(1 << mode_offset));
    lish::write_flash_string(CMD_LOGIN_SUCCESS, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return 0;
  } else {
    lish::write_flash_string(CMD_LOGIN_FAILED, shell);
    lish::write_flash_string(lish::NEWLINE, shell);
    return -1;
  }
}

// ============================================================================
// Command Descriptor (PROGMEM)
// ============================================================================

const lish::CmdDescriptor LISH_PROGMEM cmd_login_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_login>(
    CMD_LOGIN_NAME, lish::CmdPermission::Mode0 | lish::CmdPermission::HideWhenUnavailable, CMD_LOGIN_HELP);
