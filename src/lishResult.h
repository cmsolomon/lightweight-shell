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

#ifndef LISH_RESULT_H
#define LISH_RESULT_H

#include <stdint.h>

namespace lish {

/// @file lishResult.h
/// @brief Result types returned by command dispatch (Shell::run_line(), IShell::run_line()).

/// @brief Outcome of a dispatch attempt - whether a handler was reached, not whether
///        that handler (or the dispatch itself) ultimately succeeded.
///
/// @details
/// `Ok` covers more than "the command succeeded": it means dispatch found something to
/// invoke and did so - the built-in `help` command, or a registered command's handler.
/// Whether that invocation itself succeeded is carried in CommandResult::code, not here.
/// Notably, dispatch_chain()'s own parse-error handling (e.g. an unclosed quote) also
/// returns `Ok` with a nonzero code (-1), rather than a distinct status - a dispatch-level
/// failure is reported the same way a failed command's exit code would be. See
/// CommandResult's Invariants for why `code` must always be considered alongside `status`.
enum class ShellStatus : uint8_t {
  Ok,               ///< A handler was reached and invoked; see CommandResult::code for its result.
  CommandNotFound,  ///< No registered command matched the name - or it matched but is
                     ///< hidden in the current mode (hidden commands are deliberately
                     ///< reported the same as nonexistent ones; see is_hidden() in
                     ///< lishCmdPermission.h). Always paired with code == 0.
  PermissionDenied, ///< The command was found and is not hidden, but is not available in
                     ///< the current permission mode. Always paired with code == 0.
  EmptyCommand      ///< The line was empty. Only ever produced by IShell::run_line() for
                     ///< a zero-length line - the interactive path (Shell::on_submit())
                     ///< silently skips dispatch for a blank line without constructing a
                     ///< CommandResult at all, so this value never arises from interactive
                     ///< use.
  // Additional statuses can be added as needed
};

/// @brief The outcome of a single dispatch: a ShellStatus plus the invoked handler's own
///        return code.
///
/// @details
/// `status` and `code` are independent fields, not a packed/combined value - there is no
/// bit-shifting or masking involved in reading either one.
///
/// ## Invariants
/// - `code`'s meaning depends entirely on `status`. For `ShellStatus::Ok`, it is either
///   the invoked command's own return value (application-defined; 0 conventionally means
///   success, but a command may return any int8_t) or, for a dispatch_chain() parse error
///   specifically, -1 (a dispatch-level failure, not a command's). For every other status,
///   it is always 0 in current usage, though nothing in the type enforces this.
/// - `succeeded()` requires BOTH status == Ok AND code == 0 - a command that dispatched
///   fine but returned a nonzero code is not "succeeded". This is not just a display
///   distinction: dispatch_chain()'s `&&` chain-continuation logic depends on it directly
///   (a chained command only runs if the previous one's succeeded() is true), so a
///   nonzero-but-Ok result correctly halts a `&&` chain the same way a real dispatch
///   failure would.
struct CommandResult {
  ShellStatus status = ShellStatus::Ok;  ///< What dispatch did (or didn't) find/invoke.
  int8_t code = 0;                       ///< See class-level Invariants for what this means.

  /// @brief Default-constructs a successful, zero-code result.
  constexpr CommandResult() = default;

  /// @brief Constructs a result with an explicit status and code.
  /// @param s The dispatch status.
  /// @param c The handler's return code, or a dispatch-level code for a non-Ok status
  ///          (see class-level Invariants). Defaults to 0.
  constexpr CommandResult(const ShellStatus s, const int8_t c = 0) : status(s), code(c) {}

  /// @brief Whether this result represents a fully successful dispatch and invocation.
  /// @return true only if status == ShellStatus::Ok AND code == 0. A command that
  ///         dispatched successfully but returned a nonzero code is NOT considered
  ///         succeeded - see class-level Invariants.
  constexpr bool succeeded() const {
    return status == ShellStatus::Ok && code == 0;
  }
};

// Kept intentionally small (not for any bit-packing scheme - status and code are plain,
// separately-accessed fields) so CommandResult returns efficiently by value on AVR.
static_assert(sizeof(CommandResult) == 2, "CommandResult must be exactly 2 bytes");

}  // namespace lish

#endif  // LISH_RESULT_H
