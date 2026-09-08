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

/// @file lishDefs.h
/// @brief Core utility helpers (unused-parameter marker, command name hashing)
///        and shared Flash-stored UI/status/error string constants.

#ifndef LISH_DEFS_H
#define LISH_DEFS_H

#include <stddef.h>
#include <stdint.h>
#include "lishPlatform.h"

namespace lish {

/// @brief Marks a variable or parameter as intentionally unused.
///
/// @details
/// Accepts any argument by forwarding reference and does nothing with it; the
/// empty body compiles to nothing. Suppresses "unused parameter" warnings at the
/// call site without a `(void)param;` cast, e.g. in a command handler that
/// receives a context it doesn't need: `lish::unused(ctx);`.
template<typename T>
constexpr void unused(T&&) noexcept {}

/// @brief Computes FNV-1a case-insensitive hash of a command name for O(1) lookup.
///
/// @details
/// Recurses one character at a time, lowercasing each byte before folding it into
/// the hash, so two names differing only in case produce the same hash. Used by
/// CmdDescriptor::make() to precompute a command's hash at compile time, and by
/// callers doing hash-based dispatch (see CmdDescriptor::matches_hash()).
///
/// @param str Null-terminated string to hash. Must not be null - unlike command()
///            and operator[] elsewhere in this library, this function dereferences
///            str immediately with no null check.
/// @param hash Accumulator for the recursive computation; internal use only. Leave
///             at the default (the FNV-1a offset basis) when calling from outside.
/// @return The 32-bit FNV-1a hash of the lowercased string.
constexpr uint32_t hash_cmd(const char* const str, const uint32_t hash = 0x811c9dc5) {
  return (*str == '\0')
           ? hash
           : hash_cmd(str + 1, (hash ^ static_cast<uint8_t>(to_lower(*str))) * 16777619u);
}

// ============================================================================
// Shell Display & UI Constants (Stored in Flash on AVR)
// ============================================================================
LISH_FLASH_STORAGE char ANSI_DIM[] LISH_PROGMEM = "\x1B[2m";
LISH_FLASH_STORAGE char ANSI_RESET[] LISH_PROGMEM = "\x1B[0m";
LISH_FLASH_STORAGE char ANSI_CLEAR_LINE[] LISH_PROGMEM = "\x1B[2K\r";
LISH_FLASH_STORAGE char ANSI_CLEAR_TO_EOL[] LISH_PROGMEM = "\x1B[K";
/// Cursor-left escape sequence fragments: write ANSI_CURSOR_LEFT, then a decimal
/// digit count, then ANSI_CURSOR_LEFT_SUFFIX to move the cursor left N columns
/// (`ESC[<N>D`). Neither half is meaningful alone; see
/// InputProcessor::partial_redraw_from() for the only place they're combined.
LISH_FLASH_STORAGE char ANSI_CURSOR_LEFT[] LISH_PROGMEM = "\x1B[";
LISH_FLASH_STORAGE char ANSI_CURSOR_LEFT_SUFFIX[] LISH_PROGMEM = "D";


LISH_FLASH_STORAGE char NEWLINE[] LISH_PROGMEM = "\r\n";
LISH_FLASH_STORAGE char PROMPT_DEFAULT[] LISH_PROGMEM = "> ";

// ============================================================================
// Status & Error Message Constants
// ============================================================================
/// Printed as-is after a successful command (code 0). See Shell::print_status().
LISH_FLASH_STORAGE char STATUS_OK[] LISH_PROGMEM = "[OK]";

/// Bracket fragments wrapping an error message or code, e.g. "[ERROR: <text>]"
/// or "[ERROR: Code <NNN>]". Combined with one of the ERROR_* strings below (or
/// with ERROR_CODE_PREFIX + digits) in Shell::print_status(); neither fragment
/// is a complete message on its own.
LISH_FLASH_STORAGE char STATUS_ERROR_PREFIX[] LISH_PROGMEM = "[ERROR: ";
LISH_FLASH_STORAGE char STATUS_ERROR_SUFFIX[] LISH_PROGMEM = "]";

/// Shell-level dispatch errors (ShellStatus), printed between STATUS_ERROR_PREFIX
/// and STATUS_ERROR_SUFFIX. Distinct from a command's own non-zero return code,
/// which is reported via ERROR_CODE_PREFIX instead.
LISH_FLASH_STORAGE char ERROR_COMMAND_NOT_FOUND[] LISH_PROGMEM = "Command not found";
LISH_FLASH_STORAGE char ERROR_PERMISSION_DENIED[] LISH_PROGMEM = "Permission denied";

/// Fragment printed before a failed command's numeric return code (itself
/// written digit-by-digit); not a standalone message. See Shell::print_status().
LISH_FLASH_STORAGE char ERROR_CODE_PREFIX[] LISH_PROGMEM = "Code ";

/// Used by the built-in help command when given more than one argument.
LISH_FLASH_STORAGE char INVALID_ARGUMENTS[] LISH_PROGMEM = "Invalid arguments";

/// Printed when Args::parse() returns Terminator::ParseError (e.g. an unclosed
/// or misplaced quote). See Shell::dispatch_chain().
LISH_FLASH_STORAGE char ERROR_INVALID_ARGUMENT_FORMAT[] LISH_PROGMEM = "Invalid argument format";

// ============================================================================
// Built-In Command Name
// ============================================================================
/// Name of the built-in help command, which (unlike all other commands) is not
/// present in the CmdDescriptor array - it's special-cased in
/// Shell::invoke_slice() and in tab_complete(). Used there for both hashing and
/// prefix matching.
LISH_FLASH_STORAGE char HELP_COMMAND_STR[] LISH_PROGMEM = "help";

}  // namespace lish

#endif  // LISH_DEFS_H