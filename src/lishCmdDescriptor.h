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

#ifndef LISH_CMD_DESCRIPTOR_H
#define LISH_CMD_DESCRIPTOR_H

#include <stdint.h>
#include "lishPlatform.h"
#include "lishCmdPermission.h"
#include "lishDefs.h"

namespace lish {

// Forward declarations
class IShell;
class Args;

/// @brief Type erased function signature stored inside Flash command descriptors.
typedef int8_t (*CommandFunction)(const Args& args, IShell& shell, void* context);

/// @brief Lightweight compile-time command descriptor stored in Flash memory.
///
/// @details
/// CmdDescriptor encapsulates command metadata (name, permissions, help text) and a
/// typed function pointer, all stored in Flash/PROGMEM on AVR platforms. The descriptor
/// uses a Trampoline to adapt a typed handler `int8_t Handler(const Args&, IShell&, ContextType&)`
/// to a uniform erased function signature (CommandFunction) so descriptors of different
/// handler types can share one array element type.
///
/// Descriptors are created via the make() factory method, which deduces the handler type at
/// compile time, ensuring type safety while remaining constant-sized in memory. All string
/// pointers refer to Flash-stored data.
///
/// @warning On AVR, the descriptor itself and its `name`/`help` strings MUST be placed in
///          Flash via `LISH_PROGMEM`/`LISH_FLASH_STORAGE` - see make(). This is not an
///          optimization; the accessors below unconditionally read through Flash-load
///          instructions (`memcpy_P`/`pgm_read_byte`), which reinterpret whatever address
///          they're given as a Flash address. AVR is Harvard-architecture: RAM and Flash
///          are separate address spaces that both start at 0, so a RAM address handed to a
///          Flash-read instruction is a *valid but wrong* address - it silently reads
///          whatever happens to live in Flash at that numeric offset instead of your
///          string, rather than failing loudly. The bug is invisible on ARM/x86 (unified
///          address space, so the "wrong" read is also correct) and only shows up as
///          garbled command names/help text on real AVR hardware.
class CmdDescriptor {
private:
  const char* const name_;             ///< Command name (stored in Flash/PROGMEM)
  const CmdPermission permissions_;    ///< Permission requirements and visibility flags
  const char* const help_;             ///< Help text for the command (stored in Flash/PROGMEM)
  const CommandFunction func_;         ///< Erased function pointer to handler (via Trampoline)
  const uint32_t hash_;                ///< Pre-computed FNV-1a hash of command name for O(1) lookup

  /// @brief Private constructor enforced by the factory method.
  ///
  /// @details
  /// Initializes a descriptor with the given metadata and pre-computed hash. This constructor
  /// is private to ensure descriptors are created only through the make() factory method,
  /// which provides type-safe wrapping of the typed handler and computes the hash at compile time.
  ///
  /// @param name Command name (null-terminated string in Flash/PROGMEM).
  /// @param permissions Permission mode requirements and visibility flags.
  /// @param help Help/documentation text (null-terminated string in Flash/PROGMEM).
  /// @param func Erased function pointer (must be valid and non-null).
  /// @param hash Pre-computed FNV-1a hash of the command name (computed at compile time).
  constexpr CmdDescriptor(
      const char* const name,
      const CmdPermission permissions,
      const char* const help,
      const CommandFunction func,
      const uint32_t hash)
    : name_(name),
      permissions_(permissions),
      help_(help),
      func_(func),
      hash_(hash) {}

  /// @brief Type-erasing trampoline that adapts a typed command handler to CommandFunction.
  ///
  /// @details
  /// Adapts a typed handler signature `int8_t Handler(const Args&, IShell&, ContextType&)`
  /// into the erased signature `int8_t(const Args&, IShell&, void*)`. The trampoline
  /// safely casts the void* context pointer back to ContextType and forwards to the handler.
  ///
  /// @tparam ContextType The application-specific context type.
  /// @tparam Handler Function pointer with typed context parameter.
  ///
  /// ## Invariants
  /// - The void* context parameter must actually point to a ContextType instance.
  /// - Handler must be non-null and valid.
  template <typename ContextType, int8_t (*Handler)(const Args&, IShell&, ContextType&)>
  struct Trampoline {
    /// @brief Erased trampoline function that casts void* context and calls Handler.
    static int8_t run(const Args& args, IShell& shell, void* const context) {
      return Handler(args, shell, *static_cast<ContextType*>(context));
    }
  };

public:
  /// @brief Default constructor is deleted; use the make() factory method instead.
  CmdDescriptor() = delete;

  /// @brief Factory method for command handlers.
  ///
  /// @details
  /// Creates a CmdDescriptor from a typed command handler. The handler type
  /// `int8_t Handler(const Args&, IShell&, ContextType&)` is automatically deduced from
  /// the template arguments, and wrapped in a Trampoline for type erasure.
  ///
  /// The command name hash is computed at compile time using FNV-1a and stored in the descriptor,
  /// enabling O(1) hash-based command lookup without runtime computation.
  ///
  /// This method is constexpr, allowing descriptors to be created at compile time and
  /// stored in Flash/PROGMEM.
  ///
  /// @tparam ContextType Type of the application context passed to the handler. Every
  ///         command shares the same ContextType for a given Shell instance (it is fixed
  ///         by the Shell's own ContextType template parameter). If a particular command
  ///         has no use for the context, declare the handler's context parameter but leave
  ///         it unused (see @c lish::unused()) rather than introducing a different signature.
  /// @tparam Handler Function pointer with typed context parameter.
  ///
  /// @param name Null-terminated command name string (must be in Flash/PROGMEM).
  /// @param permissions Permission mode requirements and visibility flags.
  /// @param help Null-terminated help text (must be in Flash/PROGMEM).
  ///
  /// @return A new CmdDescriptor wrapping the handler with pre-computed hash.
  ///
  /// @warning `name` and `help` MUST be `LISH_FLASH_STORAGE`/`LISH_PROGMEM` strings, and the
  ///          returned CmdDescriptor MUST itself be stored with `LISH_PROGMEM` - on AVR these
  ///          are read back via Flash-load instructions unconditionally, not just when
  ///          convenient. A plain RAM string literal or a non-PROGMEM descriptor still
  ///          *compiles and links* - it just reads back garbage on real AVR hardware (see
  ///          the class-level @warning above for why), while working fine on ARM/x86. Don't
  ///          rely on Uno R4/desktop testing alone to catch a missing LISH_PROGMEM.
  ///
  /// ## Usage Example
  /// @code
  /// // cmd_echo.cpp
  /// LISH_FLASH_STORAGE char CMD_ECHO_NAME[] LISH_PROGMEM = "echo";
  /// LISH_FLASH_STORAGE char CMD_ECHO_HELP[] LISH_PROGMEM = "Echo arguments back to output";
  ///
  /// int8_t cmd_echo(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  ///   lish::unused(ctx);  // This command doesn't need the context
  ///   for (uint8_t i = 0; i < args.count(); ++i) {
  ///     lish::write_string(args[i], shell);
  ///     shell.write(' ');
  ///   }
  ///   lish::write_flash_string(lish::NEWLINE, shell);
  ///   return 0;
  /// }
  ///
  /// LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_echo_descriptor =
  ///   lish::CmdDescriptor::make<AppContext, &cmd_echo>(
  ///     CMD_ECHO_NAME, lish::CmdPermission::AllModes, CMD_ECHO_HELP);
  /// @endcode
  template <typename ContextType, int8_t (*Handler)(const Args&, IShell&, ContextType&)>
  static constexpr CmdDescriptor make(
      const char* const name,
      const CmdPermission permissions,
      const char* const help) {
    return CmdDescriptor(
        name,
        permissions,
        help,
        &Trampoline<ContextType, Handler>::run,
        hash_cmd(name)
    );
  }

  // --- Flash Accessors ---

  /// @brief Reads the command name from Flash/PROGMEM.
  ///
  /// @return Pointer to null-terminated command name string in Flash/PROGMEM.
  const char* name() const;

  /// @brief Reads the permission requirements from Flash/PROGMEM.
  ///
  /// @return CmdPermission value encoding mode availability and visibility flags.
  CmdPermission permissions() const;

  /// @brief Reads the help text from Flash/PROGMEM.
  ///
  /// @return Pointer to null-terminated help text string in Flash/PROGMEM.
  const char* help() const;

  /// @brief Executes the underlying command handler safely.
  ///
  /// @details
  /// Reads the handler function pointer from Flash/PROGMEM and invokes it with the
  /// provided arguments and context. On AVR, this performs a safe Flash read before
  /// calling the handler; on ARM/unified, this is a direct dereference.
  ///
  /// @param args Command arguments parsed from the command line.
  /// @param shell Reference to the current shell instance for command use.
  /// @param context Opaque context pointer (type determined at descriptor creation).
  ///
  /// @return Return code from the handler (typically 0 for success, non-zero for error).
  ///
  /// @note The handler is always called via the Trampoline, which manages type casting
  ///       of the context parameter if needed.
  int8_t execute(const Args& args, IShell& shell, void* const context) const;

  /// @brief Checks if the command is hidden in the given permission mode.
  ///
  /// @details
  /// A hidden command is completely omitted from help listings, tab completion,
  /// and other discovery mechanisms. A command is hidden if it has the HideWhenUnavailable
  /// flag set AND is not available in the current mode.
  ///
  /// @param current_mode The current permission/mode to check visibility against.
  ///
  /// @return true if the command should be hidden from the user, false otherwise.
  ///
  /// @see available() to check if a command can be executed
  bool hidden(const CmdPermission current_mode) const;

  /// @brief Checks if the command is available in the given permission mode.
  ///
  /// @details
  /// An available command is one that the user is permitted to execute in the
  /// current permission mode. This performs a bitwise check against the permissions
  /// field and the current mode.
  ///
  /// @param current_mode The current permission/mode to check availability against.
  ///
  /// @return true if the command can be executed in this mode, false otherwise.
  ///
  /// @note A command can be unavailable but not hidden (dim in listings), or
  ///       unavailable and hidden (not visible at all). See hidden() for the
  ///       distinction between visibility and availability.
  bool available(const CmdPermission current_mode) const;

  /// @brief Checks if the command name matches a given FNV-1a hash value.
  ///
  /// @details
  /// Performs a direct comparison of the descriptor's pre-computed FNV-1a hash against
  /// the provided hash value. The descriptor's hash is computed at compile time during
  /// descriptor creation via hash_cmd(name) and stored in Flash, eliminating any
  /// runtime hash computation.
  ///
  /// Used for O(1) command lookup by hash with zero runtime overhead.
  ///
  /// @param hash FNV-1a hash value to compare against.
  ///
  /// @return true if the descriptor's pre-computed hash matches this value, false otherwise.
  ///
  /// @note This is a simple pointer-dereference + comparison operation with no string
  ///       traversal or computation at runtime.
  bool matches_hash(const uint32_t hash) const;

  /// @brief Checks if the command name matches a given string (case-insensitive).
  ///
  /// @details
  /// Compares the command name against a given string with case-insensitive matching.
  /// Reads the Flash-stored name byte-by-byte for the comparison.
  ///
  /// @param name Null-terminated string to compare against (in RAM).
  ///
  /// @return true if the command name matches this string (case-insensitive), false otherwise.
  ///
  /// @warning The provided name parameter must be a valid, null-terminated RAM string.
  bool matches_name(const char* const name) const;
};

} // namespace lish

#endif // LISH_CMD_DESCRIPTOR_H