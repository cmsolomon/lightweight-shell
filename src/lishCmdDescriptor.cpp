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

#include "lishCmdDescriptor.h"
#include "lishCmdPermission.h"
#include "lishPlatform.h"
#include "lishDefs.h"
#include <string.h>

namespace lish {

const char* CmdDescriptor::name() const {
  return read_flash(&name_);
}

CmdPermission CmdDescriptor::permissions() const {
  return read_flash(&permissions_);
}

const char* CmdDescriptor::help() const {
  return read_flash(&help_);
}

int8_t CmdDescriptor::execute(const Args& args, IShell& shell, void* const context) const {
  CommandFunction fn = read_flash(&func_);
  if (fn) {
    return fn(args, shell, context);
  }
  return -1;
}

bool CmdDescriptor::hidden(const CmdPermission current_mode) const {
  return is_hidden(permissions(), current_mode);
}

bool CmdDescriptor::available(const CmdPermission current_mode) const {
  return is_available(permissions(), current_mode);
}

bool CmdDescriptor::matches_hash(const uint32_t hash) const {
  return read_flash(&hash_) == hash;
}

bool CmdDescriptor::matches_name(const char* const name) const {
  const char* name_ptr = this->name();
  if (!name_ptr || !name) {
    return false;
  }

  uint8_t i = 0;
  while (true) {
    char this_char = read_flash_char(name_ptr + i);
    char other_char = name[i];
    if (to_lower(this_char) != to_lower(other_char)) {
      return false;
    }
    if (this_char == '\0') {
      return true;
    }
    ++i;
  }
}

} // namespace lish