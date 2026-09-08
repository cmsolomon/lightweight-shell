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

#ifndef LISH_HISTORY_H
#define LISH_HISTORY_H

#include "lishLineBuffer.h"
#include "lishPlatform.h"
#include <stdint.h>
#include <string.h>

namespace lish {

/// @class History
/// @brief Manages a fixed-depth command history buffer with browsing capability.
///
/// Stores null-terminated command strings sequentially in a single buffer.
/// When the buffer fills, oldest entries are removed to make room for new ones.
///
/// @tparam HistoryLength Maximum number of history entries to retain (e.g., 16)
/// @tparam TypicalLineLength Expected average command length for buffer sizing (e.g., 10)
///         Actual buffer size = HistoryLength * TypicalLineLength
///
/// ## Invariants
/// - Buffer contains null-terminated strings stored sequentially
/// - Entries are stored newest-first: index 0 is the most recently pushed entry, and
///   index increases toward older entries. push() always inserts/moves an entry to
///   index 0; eviction always removes from the high-index (oldest) end.
/// - bytes_used_ = number of bytes used (including all null terminators)
/// - cursor_ == HistoryLength means not browsing; < HistoryLength means at that entry
/// - Entries are discovered by walking buffer: entry[i] is found by skipping i null terminators
template <uint8_t HistoryLength, uint8_t TypicalLineLength>
class History {
private:
  static constexpr uint16_t BUFFER_SIZE = (uint16_t)HistoryLength * TypicalLineLength;

  char history_buffer_[BUFFER_SIZE];  ///< Buffer of null-terminated strings
  uint16_t bytes_used_;               ///< Bytes used in buffer (including nulls)
  uint8_t cursor_;                    ///< Browsing index; HistoryLength = not browsing

  /// @brief Get pointer to entry at given index by walking the buffer.
  /// @return Pointer to entry string, or nullptr if index out of range or entry is not null-terminated
  const char* get_entry_ptr(const uint8_t index) const {
    const char* ptr = history_buffer_;
    for (uint8_t i = 0; i < index; ++i) {
      size_t len = strnlen(ptr, BUFFER_SIZE - (ptr - history_buffer_));
      // Verify null terminator actually exists (not just at max length)
      if (ptr + len >= history_buffer_ + BUFFER_SIZE || ptr[len] != '\0') {
        return nullptr;  // Fragment or not properly null-terminated
      }
      if (ptr + len >= history_buffer_ + bytes_used_) {
        return nullptr;  // Reached end of used buffer
      }
      ptr += len + 1;  // Skip entry and its null terminator
    }

    return ptr;
  }

  /// @brief Count number of entries in buffer by walking through them.
  uint8_t count_entries() const {
    uint8_t count = 0;
    const char* ptr = history_buffer_;
    while (ptr < history_buffer_ + bytes_used_) {
      size_t len = strnlen(ptr, BUFFER_SIZE - (ptr - history_buffer_));
      // Verify null terminator actually exists (not just at max length)
      if (ptr + len >= history_buffer_ + BUFFER_SIZE || ptr[len] != '\0') {
        break;  // Fragment or not properly null-terminated, stop counting
      }
      if (len == 0) {
        break;  // Empty entry signals end
      }
      ptr += len + 1;  // Skip entry and its null terminator
      count++;
    }
    return count;
  }

  /// @brief Get length of a null-terminated string in the buffer.
  /// Returns 0 if entry is not properly null-terminated within buffer bounds.
  uint8_t entry_length(const char* const entry) const {
    size_t len = strnlen(entry, BUFFER_SIZE - (entry - history_buffer_));
    // Verify null terminator actually exists (not just at max length)
    if (entry + len >= history_buffer_ + BUFFER_SIZE || entry[len] != '\0') {
      return 0;  // Not properly null-terminated
    }
    return (uint8_t)len;
  }

public:
  /// @brief Construct a History manager.
  /// @post buffer is zeroed, bytes_used_ = 0, cursor_ = HistoryLength (not browsing)
  History() : history_buffer_{}, bytes_used_(0), cursor_(HistoryLength) {
  }

  /// @brief Add a line buffer to history with automatic deduplication.
  ///
  /// Copies the LineBuffer content to history as a null-terminated string, handled as
  /// one of three cases:
  /// 1. **Already the most recent entry**: the buffer itself is untouched; only cursor_
  ///    is reset (exiting browsing mode counts as an observable effect even here).
  /// 2. **Duplicate exists elsewhere** (case-insensitive): moved to the front in a single
  ///    shift of the newer entries that preceded it - see @details below. Never evicts,
  ///    since a duplicate is by definition the same size as the incoming line.
  /// 3. **New entry**: prepended at the front (position 0), evicting the oldest (last)
  ///    entries first if there's not enough room.
  ///
  /// A line with no content (line_len == 0, or an empty buffer) is silently ignored.
  ///
  /// @details
  /// Case 2 relies on a key property: a duplicate match is always exactly `line_length`
  /// bytes (that's the match condition), so moving it to the front never changes
  /// bytes_used_. Rather than removing the duplicate (shifting the older entries after it
  /// left to close the gap) and then separately shifting everything right again to open
  /// room at the front - which physically moves the older-than-duplicate suffix twice for
  /// a net displacement of zero - only the newer-than-duplicate prefix is shifted right,
  /// by exactly the duplicate's size. This simultaneously closes the gap where the
  /// duplicate was and opens the exact-sized gap needed at the front; the
  /// older-than-duplicate suffix is already in its final position and is never touched.
  ///
  /// @tparam MaxLen Maximum size of the provided LineBuffer.
  /// @param line_buf Const reference to a LineBuffer to push to history.
  ///
  /// @pre line_buf.buffer[line_buf.line_len] == '\0' (null-terminated)
  /// @post On any path that actually records the line (Cases 1-3 above), cursor_ is
  ///       reset to HistoryLength (exit browsing). The two early-return exceptions -
  ///       an empty line, or a new entry too large to fit even after evicting every
  ///       other entry - leave cursor_ (and any in-progress browsing) unchanged.
  template <uint8_t MaxLen>
  void push(const LineBuffer<MaxLen>& line_buf) {
    // Ignore empty lines
    if (line_buf.line_len == 0 || line_buf.buffer[0] == '\0') {
      return;
    }

    uint8_t line_length = line_buf.line_len;
    if (line_length >= MaxLen) {
      line_length = MaxLen - 1;
    }

    // Case 1: already the most recent entry - nothing to do. Also spares the
    // duplicate search below (Case 2) for what is likely the most common case
    // in interactive use (re-submitting the last command unchanged).
    if (bytes_used_ > 0) {
      uint8_t front_len = entry_length(history_buffer_);
      if (front_len == line_length &&
          strncasecmp(history_buffer_, line_buf.buffer, line_length) == 0) {
        cursor_ = HistoryLength;  // Exit browsing mode
        return;
      }
    }

    // Case 2: duplicate exists elsewhere in history (case-insensitive)
    const char* dup_ptr = nullptr;
    {
      const char* entry = history_buffer_;
      while (entry < history_buffer_ + bytes_used_) {
        uint8_t entry_len = entry_length(entry);
        if (entry_len == line_length &&
            strncasecmp(entry, line_buf.buffer, line_length) == 0) {
          dup_ptr = entry;
          break;
        }
        entry += entry_len + 1;  // +1 for null terminator
      }
    }

    if (dup_ptr != nullptr) {
      uint16_t dup_offset = dup_ptr - history_buffer_;
      uint16_t dup_total_size = line_length + 1;  // always the same size as the new line

      if (dup_offset > 0) {
        memmove(&history_buffer_[dup_total_size], history_buffer_, dup_offset);
      }

      memcpy(history_buffer_, line_buf.buffer, line_length);
      history_buffer_[line_length] = '\0';

      cursor_ = HistoryLength;  // Exit browsing mode
      return;
    }

    // Case 3: genuinely new entry
    uint16_t space_needed = line_length + 1;

    // Evict oldest entries until there's space
    // Entries are stored newest-first at position 0, so oldest is at the end
    while (bytes_used_ + space_needed > BUFFER_SIZE && bytes_used_ > 0) {
      // Find the oldest (last) entry by walking through all entries
      const char* oldest = history_buffer_;
      const char* ptr = history_buffer_;
      while (ptr < history_buffer_ + bytes_used_) {
        oldest = ptr;
        size_t len = strnlen(ptr, BUFFER_SIZE - (ptr - history_buffer_));
        if (ptr + len >= history_buffer_ + BUFFER_SIZE || ptr[len] != '\0') {
          break;  // Fragment or unterminated, stop walking
        }
        if (len == 0) {
          break;  // Empty entry
        }
        ptr += len + 1;
      }

      // Remove the oldest entry by truncating buffer at its start
      bytes_used_ = oldest - history_buffer_;
    }

    // If still no space, cannot add this entry
    if (bytes_used_ + space_needed > BUFFER_SIZE) {
      return;
    }

    // Shift all entries toward the older end to make space at the beginning
    // (the most recent end) for the new entry
    if (bytes_used_ > 0) {
      memmove(&history_buffer_[space_needed],
              history_buffer_,
              bytes_used_);
    }

    // Write new entry at the front
    memcpy(history_buffer_, line_buf.buffer, line_length);
    history_buffer_[line_length] = '\0';
    bytes_used_ += space_needed;

    cursor_ = HistoryLength;  // Exit browsing mode
  }

  /// @brief Navigate to older history entries (up arrow).
  ///
  /// If not browsing, starts from most recent entry (index 0).
  /// If already browsing, continues to older entries (higher indices).
  ///
  /// @return true if a valid history entry was displayed; false if none found or already at oldest
  bool prev() {
    uint8_t count = count_entries();
    if (count == 0) {
      return false;
    }

    if (cursor_ == HistoryLength) {
      cursor_ = 0;  // Start from most recent
      return true;
    }

    if (cursor_ < count - 1) {
      cursor_++;
      return true;
    }

    return false;
  }

  /// @brief Navigate to newer history entries (down arrow).
  ///
  /// Moves forward to newer entries if currently browsing (toward index 0).
  /// When reaching most recent, exits browsing mode.
  ///
  /// @return true if a newer entry was displayed; false if reached end or not browsing
  bool next() {
    if (cursor_ == HistoryLength) {
      return false;  // Not browsing
    }

    if (cursor_ > 0) {
      cursor_--;
      return true;
    }

    cursor_ = HistoryLength;  // Exit browsing
    return false;
  }

  /// @brief Exit history browsing mode.
  void cancel_browsing() {
    cursor_ = HistoryLength;
  }

  /// @brief Check if currently browsing history.
  bool is_browsing() const {
    return cursor_ != HistoryLength;
  }

  /// @brief Get the currently selected or most recent history entry.
  ///
  /// @return Pointer to null-terminated string of the entry, or empty string if history is empty.
  ///         Caller should not store this pointer, as it may change with push().
  const char* current() const {
    if (bytes_used_ == 0) {
      return "";
    }

    uint8_t idx = (cursor_ == HistoryLength) ? 0 : cursor_;
    const char* entry = get_entry_ptr(idx);
    return entry ? entry : "";
  }
};

}  // namespace lish

#endif  // LISH_HISTORY_H
