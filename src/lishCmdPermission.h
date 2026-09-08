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

#ifndef LISH_CMD_PERMISSIONS_H
#define LISH_CMD_PERMISSIONS_H

#include <stdint.h>
#include <stddef.h>

namespace lish {

/// @file lishCmdPermission.h
/// @brief Role-based access control (RBAC) for shell commands.
///
/// @details
/// Defines a permission and mode system for controlling which commands are
/// available in which operational contexts. Supports 7 independent "modes"
/// (operational states: locked, user, admin, debug, etc.) plus an optional
/// visibility flag for commands unavailable in the current mode.
///
/// The system uses a single uint8_t bitmask where:
/// - **Bits 0-6**: Mode availability (7 independent modes)
/// - **Bit 7**: Visibility flag (omit from help listings when unavailable;
///   does not affect tab completion, see CmdPermission::HideWhenUnavailable)
///
/// Common patterns:
/// - A command available only in Mode0: `CmdPermission::Mode0`
/// - A command available in Mode1 or Mode2: `Mode1 | Mode2`
/// - A command available everywhere: `CmdPermission::AllModes`
/// - Hide a restricted command when unavailable: `Mode0 | HideWhenUnavailable`

/// @brief Permission bitmask for command availability and visibility.
///
/// @details
/// Encodes which "modes" (operational states) allow a command to execute,
/// plus an optional visibility flag. Commands are matched to the Shell's
/// current_mode via bitwise AND: if the result is non-zero, the command
/// is available.
///
/// ## Bit Layout (MSB to LSB)
/// - **Bit 7**: HideWhenUnavailable flag (visibility in help listings only)
/// - **Bits 6-0**: Mode0..Mode6 bits (7 operational modes)
///
/// ## Invariants
/// - **Mode bits**: Exactly one bit set per mode (Mode0=0x01, Mode1=0x02, etc.).
///   Multiple mode bits may be OR'd together to allow a command in multiple modes.
/// - **AllModes constant**: Equals 0x7F (bits 0-6 set). Represents a command
///   available in all 7 modes. Never includes the HideWhenUnavailable bit.
/// - **HideWhenUnavailable flag**: Bit 7 (0x80). When set and the command is
///   unavailable in the current mode, the command is omitted from help listings
///   (via is_hidden(), see below) rather than shown dimmed. Has no effect if the
///   command is available. Note this flag only affects help listings: tab
///   completion filters candidates by available() directly and never consults
///   is_hidden(), so unavailable commands are always excluded from completion
///   suggestions regardless of whether this flag is set.
/// - **None constant**: Equals 0x00. A command with this permission is never
///   available in any mode, but is NOT hidden: since None sets no bits at all,
///   it does not set the HideWhenUnavailable flag either, so hidden() always
///   returns false for it. Such a command is listed (dimmed via ANSI_DIM, since
///   it's unavailable) rather than omitted. Combine with HideWhenUnavailable
///   explicitly (`None | HideWhenUnavailable`) to make a permanently-hidden
///   command. Used as a sentinel or default value.
/// - **Bitwise operations**: & (AND), | (OR), ~ (NOT) are overloaded for
///   CmdPermission and return CmdPermission results, preserving type safety.
/// - **Immutability**: CmdPermission values are typically compile-time constants
///   and should not be modified at runtime.
///
/// ## Example
/// @code
/// // Available only in Mode0 (e.g., initialization/setup mode)
/// constexpr auto init_cmd_perm = lish::CmdPermission::Mode0;
///
/// // Available in Mode1 (user) or Mode2 (power-user)
/// constexpr auto user_cmd_perm = lish::CmdPermission::Mode1 | lish::CmdPermission::Mode2;
///
/// // Available in every mode; never unavailable, so never hidden or dimmed
/// constexpr auto help_cmd_perm = lish::CmdPermission::AllModes;
///
/// // Debug-only, hidden when not in debug mode
/// constexpr auto debug_cmd_perm = lish::CmdPermission::Mode6 | lish::CmdPermission::HideWhenUnavailable;
/// @endcode
enum class CmdPermission : uint8_t {
  /// No permissions; command is never available.
  None = 0x00,

  /// Modes (bits 0-6): Each represents an independent operational state.
  /// Applications define the semantics (e.g., locked, user, admin, debug).
  Mode0 = 1 << 0,  ///< Operational mode 0 (e.g., setup/initialization).
  Mode1 = 1 << 1,  ///< Operational mode 1 (e.g., user).
  Mode2 = 1 << 2,  ///< Operational mode 2 (e.g., power-user).
  Mode3 = 1 << 3,  ///< Operational mode 3 (e.g., technician).
  Mode4 = 1 << 4,  ///< Operational mode 4 (e.g., admin).
  Mode5 = 1 << 5,  ///< Operational mode 5 (e.g., service/factory).
  Mode6 = 1 << 6,  ///< Operational mode 6 (e.g., debug).

  /// All modes enabled (bits 0-6 set; 0x7F). Highest common denominator.
  /// Command is available in every mode. Note: does NOT include HideWhenUnavailable.
  AllModes = 0x7F,

  /// Visibility flag (bit 7, 0x80). When set, the command is omitted from
  /// help listings if unavailable in the current mode (rather than shown
  /// dimmed). No effect if the command is available. Does not affect tab
  /// completion, which always excludes unavailable commands regardless of
  /// this flag (see is_hidden() for the distinction). Often OR'd with mode
  /// bits: `Mode0 | HideWhenUnavailable` = "only in Mode0, hidden in all others".
  HideWhenUnavailable = 0x80
};

// ============================================================================
// Bitwise Operators (C++11 constexpr)
// ============================================================================

/// @brief Bitwise AND operator for CmdPermission.
///
/// @param lhs Left-hand side permission.
/// @param rhs Right-hand side permission.
/// @return Result of bitwise AND with type CmdPermission.
///
/// ## Invariants
/// - Commutative: `(a & b) == (b & a)`.
/// - Associative: `((a & b) & c) == (a & (b & c))`.
/// - Idempotent: `(a & a) == a`.
constexpr CmdPermission operator&(const CmdPermission lhs, const CmdPermission rhs) {
  return static_cast<CmdPermission>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
}

/// @brief Bitwise OR operator for CmdPermission.
///
/// @param lhs Left-hand side permission.
/// @param rhs Right-hand side permission.
/// @return Result of bitwise OR with type CmdPermission.
///
/// ## Invariants
/// - Commutative: `(a | b) == (b | a)`.
/// - Associative: `((a | b) | c) == (a | (b | c))`.
/// - Idempotent: `(a | a) == a`.
constexpr CmdPermission operator|(const CmdPermission lhs, const CmdPermission rhs) {
  return static_cast<CmdPermission>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
}

/// @brief Bitwise NOT (complement) operator for CmdPermission.
///
/// @param val Value to invert.
/// @return Bitwise complement with type CmdPermission (all 8 bits flipped).
///
/// @note Flips ALL bits including bit 7. For mode filtering, usually follow with
/// AND against AllModes to zero bit 7: `(~perm) & AllModes`.
constexpr CmdPermission operator~(const CmdPermission val) {
  return static_cast<CmdPermission>(~static_cast<uint8_t>(val));
}

// ============================================================================
// Permission Testing Predicates
// ============================================================================

/// @brief Converts a CmdPermission to a boolean (true if any bit is set).
///
/// @param perm Permission value to test.
/// @return true if perm is non-zero (at least one bit set); false if perm is None.
///
/// ## Invariants
/// - `to_bool(None) == false`.
/// - `to_bool(any other value) == true`.
constexpr bool to_bool(const CmdPermission perm) {
  return static_cast<uint8_t>(perm) != 0;
}

/// @brief Checks if a command with given permissions is available in the current mode.
///
/// @details
/// Returns true if the command's allowed modes overlap with the current_mode.
/// Specifically: `is_available(perm, mode)` iff `(mode & (perm & AllModes)) != 0`.
///
/// The `& AllModes` filters out the HideWhenUnavailable bit (bit 7), ensuring
/// availability checks depend only on the 7 mode bits.
///
/// @param perm The command's permission bits (may include HideWhenUnavailable).
/// @param current_mode The Shell's current mode. **Invariant**: Only one mode bit
///                     (Mode0-Mode6) should be set at a time; current_mode should be
///                     exactly one of Mode0, Mode1, ..., Mode6. Behavior is undefined
///                     if multiple mode bits are set simultaneously.
/// @return true if the command can execute in current_mode; false otherwise.
///
/// ## Invariants
/// - A command with permission `None` is never available (always returns false).
/// - A command with permission `AllModes` is always available (always returns true).
/// - `is_available(perm, Mode_X)` depends only on bits 0-6 of perm, ignoring bit 7.
/// - `is_available(perm, mode)` depends only on whether perm's and mode's bit-0-6
///   sets intersect - not on the setting of the visibility flag (bit 7) in either.
/// - **Single-mode invariant**: current_mode must represent exactly one active mode.
///   The Shell maintains `current_mode` as a power of 2 (Mode0=0x01, Mode1=0x02, etc.),
///   never as a multi-bit combination.
///
/// ## Example
/// @code
/// constexpr auto perm = Mode0 | Mode1;  // Available in Mode0 or Mode1
/// static_assert(is_available(perm, Mode0));  // ✓ Available
/// static_assert(is_available(perm, Mode1));  // ✓ Available
/// static_assert(!is_available(perm, Mode2)); // ✗ Not available in Mode2
/// static_assert(is_available(perm | HideWhenUnavailable, Mode0)); // ✓ Bit 7 ignored
/// @endcode
constexpr bool is_available(const CmdPermission perm, const CmdPermission current_mode) {
  return to_bool(current_mode & (perm & CmdPermission::AllModes));
}

/// @brief Checks if a command should be omitted from help listings.
///
/// @details
/// Returns true if BOTH conditions hold:
/// 1. The command has the HideWhenUnavailable flag set (bit 7).
/// 2. The command is NOT available in the current mode (is_available returns false).
///
/// If either condition is false, the command is listed in help (though an
/// available-but-otherwise-visible command with HideWhenUnavailable unset that
/// happens to be unavailable is still listed, shown dimmed via ANSI_DIM).
///
/// This predicate governs help listings only. Tab completion does not call
/// is_hidden() at all - it filters candidates by is_available() directly, so
/// unavailable commands are always excluded from completion regardless of
/// whether HideWhenUnavailable is set.
///
/// @param perm The command's permission bits (including visibility flag).
/// @param current_mode The Shell's current mode.
/// @return true if the command should be omitted from help listings; false if
///         it should be listed (dimmed or not, depending on availability).
///
/// ## Invariants
/// - `is_hidden` always implies `!is_available` (command is not executable).
/// - A command with HideWhenUnavailable flag set but available is still listed.
/// - A command without the HideWhenUnavailable flag is always listed, even if
///   unavailable (dimmed rather than omitted).
/// - `is_hidden(perm, mode) == false` if `is_available(perm, mode) == true`.
///
/// ## Example
/// @code
/// constexpr auto debug_perm = Mode6 | HideWhenUnavailable;
/// // In Mode6: available AND visible in help
/// static_assert(!is_hidden(debug_perm, Mode6));
/// // In Mode0: not available AND hidden from help (restrictive)
/// static_assert(is_hidden(debug_perm, Mode0));
///
/// // Without HideWhenUnavailable, always visible even when unavailable
/// constexpr auto always_visible_perm = Mode6;  // No HideWhenUnavailable
/// static_assert(!is_hidden(always_visible_perm, Mode0));  // Hidden bit not set
/// @endcode
constexpr bool is_hidden(const CmdPermission perm, const CmdPermission current_mode) {
  return to_bool(perm & CmdPermission::HideWhenUnavailable) && !is_available(perm, current_mode);
}
}
#endif  // LISH_CMD_PERMISSIONS_H