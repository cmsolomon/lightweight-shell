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

#include "lishTabComplete.h"
#include "lishPlatform.h"
#include "lishDefs.h"

namespace lish {

// See declaration in lishTabComplete.h for full documentation.
bool extract_command_segment(const char* const buffer, const uint8_t buffer_len,
                             uint8_t& segment_start, uint8_t& prefix_len) {
  // Find the start of the current segment (after last ; or &&)
  segment_start = 0;
  for (uint8_t i = 0; i < buffer_len; ++i) {
    if (i + 1 < buffer_len && buffer[i] == '&' && buffer[i + 1] == '&') {
      segment_start = i + 2;
      i++;  // Skip the second &
    } else if (buffer[i] == ';') {
      segment_start = i + 1;
    }
  }

  // Skip leading spaces in the segment
  while (segment_start < buffer_len && buffer[segment_start] == ' ') {
    segment_start++;
  }

  // Find the end of the command word (first space or end of buffer)
  uint8_t cmd_end = segment_start;
  while (cmd_end < buffer_len && buffer[cmd_end] != ' ') {
    cmd_end++;
  }

  // If there's a space, the command portion is done - do nothing
  if (cmd_end < buffer_len && buffer[cmd_end] == ' ') {
    return false;
  }

  // Extract the partial command length
  prefix_len = cmd_end - segment_start;
  if (prefix_len == 0) {
    return false;  // No command to complete
  }

  return true;
}

// See declaration in lishTabComplete.h for full documentation.
bool prefix_match(const char* const buffer, const uint8_t buffer_len,
                  const char* const command) {
  uint8_t command_len = flash_string_length(command);

  // Buffer length must not exceed command length for a prefix match
  if (buffer_len > command_len) {
    return false;
  }

  // Compare buffer bytes with command bytes (case-insensitive)
  for (uint8_t i = 0; i < buffer_len; ++i) {
    if (to_lower(buffer[i]) != to_lower(read_flash_char(command + i))) {
      return false;
    }
  }

  return true;
}

// See declaration in lishTabComplete.h for full documentation.
bool complete_command_in_buffer(char* const buffer, const uint8_t segment_start, const char* const full_command, const uint8_t max_len, uint8_t& new_len, uint8_t& new_cursor) {
  // Calculate new buffer length: segment_start + full_command + space
  const uint8_t full_len = flash_string_length(full_command);
  uint16_t completion_len = segment_start + full_len + 1;

  // Check if it fits (accounting for null terminator)
  if (completion_len >= max_len) {
    return false;
  }

  // Copy the full command from FLASH to the buffer
  for (uint8_t position = 0; position < full_len; ++position) {
    buffer[segment_start + position] = read_flash_char(full_command + position);
  }

  // Add space after the command
  buffer[segment_start + full_len] = ' ';
  buffer[segment_start + full_len + 1] = '\0';

  new_len = completion_len;
  new_cursor = completion_len;

  return true;
}

}  // namespace lish
