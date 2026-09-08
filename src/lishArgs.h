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

#ifndef LISH_ARGS_H
#define LISH_ARGS_H

#include <stdint.h>
#include <stddef.h>

namespace lish {

/// @file lishArgs.h
/// @brief Argument parser and tokenizer for lish commands.
///
/// @details
/// The Args class handles tokenization and argument parsing for the Shell's line buffer.
/// It operates without template specialization on buffer sizes or line lengths.
/// Tokens are separated by whitespace; commands are separated by `;` (sequential) or
/// `&&` (conditional). All separator and whitespace characters are replaced with nulls
/// in-place to create null-terminated tokens.
///
/// An argument may be wrapped in matching double or single quotes (`"..."` or `'...'`)
/// to include spaces or the other quote character verbatim; the enclosing quotes are
/// stripped from the resulting token. A quote must start at the beginning of a token and
/// its closing quote must be immediately followed by a separator or end-of-line — any other
/// placement (a quote mid-token, an unclosed quote, or trailing text after the closing quote)
/// causes parse() to return Terminator::ParseError.
///
/// @par Invariants & Safety Guarantees:
/// - **Null-Termination Invariant:** The tokenizer replaces whitespace, command
///   chain separators (`;`, `&&`), quote delimiters (`"`, `'`), and trailing line endings
///   with null terminators (`\0`) in-place. Every token returned by operator[] or command()
///   is guaranteed to be a valid, null-terminated C-string.
/// - **Bounds Invariant:** Token indexing is guarded by arg_count_ in O(1) time. Requesting an
///   index where `index >= count()` safely returns `nullptr` without reading past the parsed
///   tokens. (command() uses a different convention: it returns `""`, never `nullptr`, when
///   the buffer is null, exhausted, or the command name is empty.)
/// - **Token Existence Invariant:** When `index < count()`, the tokenizer guarantees that the
///   requested token exists within the line buffer slice, ensuring bounded string scanning.
/// - **Offset Invariant:** After parse(), the slice_offset parameter points to the start of
///   the next command segment (if Semicolon or LogicalAnd) or to the end-of-line marker.
///   On ParseError, slice_offset is left unmodified (still the segment's original start);
///   arg_count_ may also reflect only a partial scan up to the point of failure, so command(),
///   count(), and operator[] should not be relied on after a ParseError.
class Args {
    template <
        typename IOAdapter,
        typename LineBufferType,
        typename LineEditorType,
        typename HistoryType,
        typename ContextType,
        typename PromptCallbackType,
        size_t NumCommands,
        uint8_t MaxLineLength
    >
    friend class Shell;

public:
    /// @brief Indicates how a parsed command segment was terminated.
    enum class Terminator {
      Semicolon,    ///< Found `;` — continue with next command (sequential execution)
      LogicalAnd,   ///< Found `&&` — continue only if this command succeeded
      EndOfLine,    ///< Found end of buffer (`\0`) — no more commands
      ParseError,   ///< Malformed argument (e.g., unclosed quote)
    };

    /// @brief Constructs an Args parser bound to a line buffer.
    /// @param line_buffer Pointer to the Shell's internal line buffer (will be mutated).
    /// @param max_length Maximum length of the buffer.
    explicit constexpr Args(char* const line_buffer, const uint8_t max_length)
        : buffer_(line_buffer), max_length_(max_length), offset_(0), arg_count_(0) {}

    /// @brief Parses the next command segment from the buffer.
    /// @details
    /// Tokenizes whitespace-separated arguments starting from slice_offset, scanning for
    /// separators (`;` or `&&`) or end-of-line. Replaces whitespace and separators with
    /// nulls in-place. Updates slice_offset to indicate the next parse position.
    ///
    /// @param[in,out] slice_offset
    /// - Input: Index in buffer where this command segment begins
    /// - Output: Updated based on terminator:
    ///   - If Semicolon or LogicalAnd: points to first char after the separator (start of next command)
    ///   - If EndOfLine: points to the null terminator (end of buffer)
    ///   - If ParseError: left unmodified (still the segment's original start)
    /// @return Terminator enum indicating how the segment ended
    Terminator parse(uint8_t& slice_offset);

    /// @brief Gets the command name (token 0) for the active command slice.
    /// @return Pointer to null-terminated command name string, or "" if buffer is empty.
    const char* command() const;

    /// @brief Gets the argument count for the active command slice.
    /// @return O(1) argument count excluding the command name.
    uint8_t count() const;

    /// @brief Accesses parameter by zero-based index (0 = first parameter after command).
    /// @param index Zero-based argument index.
    /// @return Pointer to null-terminated argument string, or nullptr if index >= count().
    const char* operator[](const uint8_t index) const;

private:
    char* buffer_;           ///< Line buffer (mutable, will be null-delimited)
    uint8_t max_length_;     ///< Maximum buffer length
    uint8_t offset_;         ///< Start of current command slice
    uint8_t arg_count_;      ///< Argument count for current slice
};

} // namespace lish

#endif // LISH_ARGS_H