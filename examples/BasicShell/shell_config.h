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
  shell_config.h - Single place defining what this shell app is made of: the
  application context type, every command it registers, and the command
  table itself. BasicShell.ino includes just this file and uses the
  resulting `commands` array directly with make_shell() - everything about
  which commands exist is defined here, not split across the sketch.

  Adding a new command to this example: create its cmd_X.h/cmd_X.cpp pair
  (see any existing cmd_*.h for the pattern), add #include "cmd_X.h" below,
  then add &cmd_X_descriptor to the commands[] array below. No changes to
  BasicShell.ino are needed.

  `commands` is declared const (via LISH_FLASH_STORAGE/LISH_PROGMEM), which
  gives it internal linkage at namespace scope in C++ - safe to define
  directly in a header even if it were ever included from more than one
  translation unit (each would get its own copy, not a duplicate-symbol
  linker error), though in this sketch only BasicShell.ino includes it.
*/

#ifndef SHELL_CONFIG_H
#define SHELL_CONFIG_H

#include <lish.h>
#include "app_context.h"

#include "cmd_echo.h"
#include "cmd_login.h"
#include "cmd_logout.h"
#include "cmd_mode.h"
#include "cmd_sudo.h"
#include "cmd_uptime.h"
#include "cmd_whoami.h"

// Array of pointers to the command descriptors defined in each cmd_*.cpp
// above. Both the pointer array and the descriptors themselves are stored in
// PROGMEM (Flash), minimizing RAM usage to zero for command metadata.
// make_shell() (in BasicShell.ino) deduces NumCommands from this array's
// size automatically.
LISH_FLASH_STORAGE lish::CmdDescriptor* const LISH_PROGMEM commands[] = {
  &cmd_echo_descriptor,
  &cmd_uptime_descriptor,
  &cmd_mode_descriptor,
  &cmd_sudo_descriptor,
  &cmd_login_descriptor,
  &cmd_logout_descriptor,
  &cmd_whoami_descriptor,
};

#endif
