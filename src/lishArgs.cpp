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

#include "lishArgs.h"

namespace lish {

Args::Terminator Args::parse(uint8_t& slice_offset) {
  if (!buffer_ || slice_offset >= max_length_) {
    arg_count_ = 0;
    offset_ = slice_offset;
    return Terminator::EndOfLine;
  }

  offset_ = slice_offset;
  arg_count_ = 0;
  bool in_token = false;

  for (uint8_t i = slice_offset; i < max_length_; ++i) {
    char c = buffer_[i];

    // Whitespace: null-terminate token if needed
    if (c == ' ' || c == '\t') {
      if (!in_token) {
        // Replace all whitespace (leading or between tokens) with null
        buffer_[i] = '\0';
        if (arg_count_ == 0) {
          offset_ = i + 1;
        }
      } else {
        // We're in a token, so this whitespace ends it
        buffer_[i] = '\0';
        in_token = false;
      }
    }
    // Sequential separator: `;`
    else if (c == ';') {
      buffer_[i] = '\0';
      if (arg_count_ > 0) {
        slice_offset = i + 1;
        return Terminator::Semicolon;
      }
      offset_ = i + 1;
      in_token = false;
    }
    // Conditional separator: `&&`
    else if (c == '&' && i + 1 < max_length_ && buffer_[i + 1] == '&') {
      buffer_[i] = '\0';
      buffer_[i + 1] = '\0';
      if (arg_count_ > 0) {
        slice_offset = i + 2;
        return Terminator::LogicalAnd;
      }
      offset_ = i + 2;
      ++i;
      in_token = false;
    }
    // End of line (null terminator found)
    else if (c == '\0') {
      slice_offset = i;
      return Terminator::EndOfLine;
    }
    // Quote character
    else if (c == '"' || c == '\'') {
      // Quotes must be at the start of a token, not in the middle
      if (in_token) {
        return Terminator::ParseError;  // Quote found in middle of unquoted token
      }

      char quote_char = c;
      uint8_t token_start = i + 1;

      // Find the closing quote
      uint8_t j = token_start;
      for (; j < max_length_; ++j) {
        if (buffer_[j] == quote_char) {
          // Found closing quote - verify what comes after
          uint8_t after_quote = j + 1;
          if (after_quote >= max_length_ || buffer_[after_quote] == '\0' ||
              buffer_[after_quote] == ' ' || buffer_[after_quote] == '\t' ||
              buffer_[after_quote] == ';' ||
              (buffer_[after_quote] == '&' && after_quote + 1 < max_length_ &&
               buffer_[after_quote + 1] == '&')) {
            // Valid: closing quote is followed by separator or EOL
            arg_count_++;
            buffer_[j] = '\0';  // Replace closing quote with null
            buffer_[i] = '\0';  // Replace opening quote with null
            in_token = true;
            i = j;  // Move past closing quote
            break;
          } else {
            // Closing quote followed by non-separator character
            return Terminator::ParseError;
          }
        } else if (buffer_[j] == '\0') {
          // Reached EOL without closing quote
          return Terminator::ParseError;
        }
      }
      if (j >= max_length_) {
        // No closing quote found
        return Terminator::ParseError;
      }
    }
    // Regular character: start token if needed
    else {
      if (!in_token) {
        arg_count_++;
        in_token = true;
      }
    }
  }

  // Reached end of buffer without explicit null terminator
  slice_offset = max_length_;
  return Terminator::EndOfLine;
}

const char* Args::command() const {
  if (!buffer_ || offset_ >= max_length_) {
    return "";
  }
  const char* cmd = &buffer_[offset_];
  return (*cmd != '\0') ? cmd : "";
}

uint8_t Args::count() const {
  // arg_count_ is total tokens (including command); return arguments only
  return arg_count_ > 0 ? arg_count_ - 1 : 0;
}

const char* Args::operator[](const uint8_t index) const {
  // arg_count_ is total tokens; arguments are tokens 1..arg_count_-1 (token 0 is command)
  if (arg_count_ == 0 || index >= arg_count_ - 1 || !buffer_) {
    return nullptr;
  }

  uint8_t current_token = 0;
  bool in_token = false;

  for (uint8_t i = offset_; i < max_length_; ++i) {
    char c = buffer_[i];
    if (c == '\0') {
      in_token = false;
    } else if (!in_token) {
      if (current_token == (index + 1)) {
        return &buffer_[i];
      }
      current_token++;
      in_token = true;
    }
  }
  return nullptr;
}

}  // namespace lish
