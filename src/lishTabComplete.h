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

#ifndef LISH_TAB_COMPLETE_H
#define LISH_TAB_COMPLETE_H

#include "lishLineBuffer.h"
#include "lishCmdPermission.h"
#include "lishCmdDescriptor.h"
#include "lishPlatform.h"
#include "lishDefs.h"
#include <stdint.h>
#include <limits.h>

namespace lish {

/// @file lishTabComplete.h
/// @brief Tab completion for command names.
///
/// @details
/// tab_complete() is the only entry point called from outside this file (by
/// InputProcessor::handle_tab_complete()); the other three functions are implementation
/// helpers, declared here and defined in lishTabComplete.cpp, kept non-templated to avoid
/// duplicating their code per Shell/InputProcessor instantiation.
///
/// ## Invariants
/// - Candidates are filtered by CmdDescriptor::available() only - hidden() is never
///   checked, so an unavailable command is always excluded from completion regardless of
///   whether it also has HideWhenUnavailable set (unlike help listings, which use
///   hidden() specifically to distinguish "omit" from "show dimmed"; see is_hidden()'s
///   doc in lishCmdPermission.h).
/// - Throughout tab_complete(), an index/match value equal to NumCommands is a sentinel
///   representing the built-in "help" pseudo-entry (absent from cmd_ptrs itself) - real
///   commands occupy indices [0, NumCommands).
/// - Where a command's identity must be confirmed after a hash-based check (the
///   help/array-entry collision guard), matches_hash() is always followed by
///   matches_name() - a hash collision between two distinct command names is a real,
///   tested possibility (see lishHelp.h's Invariants), not just theoretical.

/// @brief Checks if a RAM buffer matches the start of a FLASH command string (case-insensitive).
///
/// @details
/// Compares buffer (not necessarily null-terminated) against a null-terminated command
/// string in FLASH, case-insensitively.
///
/// @param buffer Pointer to buffer in RAM.
/// @param buffer_len Length of buffer to compare.
/// @param command Pointer to null-terminated command string in FLASH.
/// @return true if buffer matches the first buffer_len characters of command
///         (case-insensitive); false otherwise, or if buffer_len exceeds command's length.
bool prefix_match(const char* const buffer, const uint8_t buffer_len,
                  const char* const command);

/// @brief Extracts the command segment from a line buffer for tab completion.
///
/// @details
/// Finds the start of the current segment (after the last `;` or `&&` in the whole
/// buffer, or 0 if there is none), then skips any leading spaces within that segment,
/// then extracts the partial command from there to the first space or end of buffer.
/// Returns false if the command portion is already complete (a space follows it) or is
/// empty.
///
/// @param buffer Pointer to line buffer.
/// @param buffer_len Length of line buffer.
/// @param segment_start Output: index where the command starts (after any leading spaces).
/// @param prefix_len Output: length of the partial command to complete.
///
/// @return true if a valid partial command was found, false if complete or empty.
bool extract_command_segment(const char* const buffer, const uint8_t buffer_len,
                             uint8_t& segment_start, uint8_t& prefix_len);

/// @brief Completes a partial command in the line buffer.
///
/// @details
/// Replaces the partial command at segment_start with the full command name (read from
/// Flash via flash_string_length()/read_flash_char()), adds a trailing space, and updates
/// buffer length and cursor. Returns false without modifying the buffer if the completed
/// text wouldn't fit (leaving room for the null terminator).
///
/// @param buffer Line buffer to modify (in RAM).
/// @param segment_start Index where the command starts.
/// @param full_command Full command name (in FLASH).
/// @param max_len Maximum buffer length (capacity, including the null terminator).
/// @param new_len Output: updated buffer length (including the trailing space, excluding
///                the null terminator). Unchanged if this returns false.
/// @param new_cursor Output: updated cursor position (same value as new_len - the cursor
///                   ends up right after the trailing space). Unchanged if this returns
///                   false.
///
/// @return true if completion succeeded, false if it doesn't fit.
bool complete_command_in_buffer(char* const buffer, const uint8_t segment_start, const char* const full_command, const uint8_t max_len, uint8_t& new_len, uint8_t& new_cursor);

/// @brief Completes or lists matching command names for the current line buffer.
///
/// @details
/// Extracts the partial command at the cursor (extract_command_segment()) and matches it
/// against every available command in cmd_ptrs plus the built-in "help" - see file-level
/// Invariants for the exact filtering/collision rules. A single match is completed in
/// place (complete_command_in_buffer()); multiple matches are written to `io`, one per
/// line, instead of modifying the buffer.
///
/// @tparam MaxLineLength Capacity of line_buf's underlying buffer.
/// @tparam NumCommands Number of entries in cmd_ptrs.
/// @tparam IOAdapter I/O adapter type for writing a multiple-matches listing.
///
/// @param line_buf The interactive line buffer to read the partial command from, and
///                 (on a single match) complete in place.
/// @param cmd_ptrs Array of command descriptor pointers; entries may be nullptr for an
///                 unregistered slot.
/// @param current_mode Current shell permission mode, used to filter candidates.
/// @param io I/O adapter to write a multiple-matches listing to, if applicable.
///
/// @return true if line_buf was modified (single match completed) OR candidates were
///         written to io (multiple matches) - either way, the caller should redraw the
///         line. false if there was no partial command to complete, or no match at all -
///         nothing was written or modified.
template<uint8_t MaxLineLength, size_t NumCommands, typename IOAdapter>
bool tab_complete(
  LineBuffer<MaxLineLength>& line_buf,
  const CmdDescriptor* const (&cmd_ptrs)[NumCommands],
  const CmdPermission current_mode,
  IOAdapter& io) {

  // Step 1: Extract the command segment to complete
  uint8_t segment_start = 0;
  uint8_t prefix_len = 0;
  if (!extract_command_segment(line_buf.buffer, line_buf.line_len, segment_start, prefix_len)) {
    return false;  // No partial command to complete
  }

  const char* prefix = &line_buf.buffer[segment_start];

  // Step 2: Count matches and identify single match (if any)
  uint16_t match_idx = UINT16_MAX;
  bool multiple_matches = false;

  // Check if "help" matches
  if (prefix_match(prefix, prefix_len, HELP_COMMAND_STR)) {
    match_idx = NumCommands;
  }

  // Check all commands in the array
  for (size_t i = 0; i < NumCommands; ++i) {
    const CmdDescriptor* ptr = read_flash<const CmdDescriptor*>(&cmd_ptrs[i]);
    if (!ptr || !ptr->available(current_mode)) {
      continue;
    }

    const char* cmd_name = ptr->name();
    if (cmd_name && prefix_match(prefix, prefix_len, cmd_name)) {
      if (match_idx == UINT16_MAX) {
        match_idx = i;
      } else if (match_idx == NumCommands && ptr->matches_hash(hash_cmd("help")) && ptr->matches_name("help")) {
        // Avoid double-counting: if "help" was already matched by hardcoded check,
        // and this is also "help", just update to use the array index instead.
        // Uses ptr's own safe accessors with a RAM literal rather than
        // string_matches(cmd_name, HELP_COMMAND_STR, false): cmd_name is a Flash
        // pointer (from CmdDescriptor::name()) and, with use_flash_read=false,
        // string_matches() would have direct-dereferenced it - reading the wrong
        // address space on AVR (see string_matches()'s Preconditions in lishPlatform.h).
        match_idx = i;
      } else {
        multiple_matches = true;
        break;
      }
    }
  }

  // Step 3A: No matches - do nothing
  if (match_idx == UINT16_MAX) {
    return false;
  }

  // Step 3B: Single match - complete it
  if (!multiple_matches) {
    const char* full_command = nullptr;
    uint8_t full_len = 0;

    if (match_idx == NumCommands) {
      full_command = HELP_COMMAND_STR;
      full_len = flash_string_length(full_command);
    } else {
      // Neither ptr nor ptr->name() is null-checked here: match_idx was only ever set
      // (in Step 2 above) when this same array slot's descriptor was already confirmed
      // non-null, and a non-null descriptor's name() can't be null either (see the
      // matching note in Step 3C above). cmd_ptrs doesn't change within a single
      // tab_complete() call, so both hold here too.
      const CmdDescriptor* ptr = read_flash<const CmdDescriptor*>(&cmd_ptrs[match_idx]);
      full_command = ptr->name();
      full_len = flash_string_length(full_command);
    }

    return complete_command_in_buffer(line_buf.buffer, segment_start, full_command, MaxLineLength,
                                      line_buf.line_len, line_buf.cursor_pos);
  }

  // Step 3C: Multiple matches - list them
  for (size_t cmd_index = 0; cmd_index <= NumCommands; ++cmd_index) {
    const char* cmd_name = nullptr;
    if (cmd_index < NumCommands) {
      const CmdDescriptor* const cmd = read_flash<const CmdDescriptor*>(&cmd_ptrs[cmd_index]);
      // Skip an unregistered slot or a command unavailable in the current mode. Both must
      // be checked here (matching Step 2's search loop above) - without the null check,
      // cmd_name would stay null and reach prefix_match() below, which calls
      // flash_string_length() on it: undefined behavior (a crash on native builds via
      // strlen(nullptr); garbage flash reads from address 0 on AVR).
      if (!cmd || !cmd->available(current_mode)) {
        continue;
      }
      // name() can't actually be null for a successfully-constructed descriptor -
      // CmdDescriptor::make() calls hash_cmd(name) unconditionally, which itself
      // dereferences name, so a null name would already have failed at construction
      // (hash_cmd is constexpr, so this fails to compile rather than merely being hard
      // to test). Not re-checked here for that reason.
      cmd_name = cmd->name();
    } else {
      cmd_name = HELP_COMMAND_STR;
    }

    if (!prefix_match(prefix, prefix_len, cmd_name)) {
      continue;
    }

    write_flash_string(cmd_name, io);
    write_flash_string(NEWLINE, io);
  }

  return true;
}

}  // namespace lish

#endif  // LISH_TAB_COMPLETE_H
