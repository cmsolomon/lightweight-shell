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

#ifndef LISH_PLATFORM_H
#define LISH_PLATFORM_H

#include <stdint.h>
#include <string.h>

/// @file lishPlatform.h
/// @brief Cross-platform abstraction layer for hardware memory architectures.
///
/// @details
/// LISH_PROGMEM and LISH_FLASH_STORAGE default to AVR or unified-address-space
/// definitions based on __AVR__, but both are guarded by #ifndef: a build targeting
/// a platform with different Flash semantics can `#define` either one (e.g. via a
/// compiler flag, or before including this header) to override the default entirely,
/// without editing library source.

#if defined(__AVR__)
  #include <avr/pgmspace.h>
#endif

#ifndef LISH_PROGMEM
  #if defined(__AVR__)
    /// @brief Attribute to force data placement into AVR Flash space.
    #define LISH_PROGMEM PROGMEM
  #else
    /// @brief No-op on unified address space architectures.
    #define LISH_PROGMEM
  #endif
#endif

#ifndef LISH_FLASH_STORAGE
  #if defined(__AVR__)
    /// @brief Storage qualifier for Flash constants (const for AVR PROGMEM).
    #define LISH_FLASH_STORAGE const
  #else
    /// @brief Storage qualifier for Flash constants (constexpr const for ARM/unified, ensures const-correctness).
    #define LISH_FLASH_STORAGE constexpr const
  #endif
#endif

namespace lish {

/// @brief Converts an ASCII uppercase letter to lowercase.
///
/// @param c The character to convert.
/// @return `c + 32` if `c` is 'A'-'Z'; otherwise `c` unchanged (non-letters and
///         already-lowercase letters pass through as-is).
constexpr char to_lower(const char c) {
  return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
}

/// @brief Reads a value of type T from Flash memory safely across platforms.
///
/// @details
/// On AVR (Harvard architecture), reads the value from Flash memory using
/// memcpy_P(). On ARM and unified address space platforms, performs a direct
/// pointer dereference since Flash and RAM share the same address space.
///
/// ## Preconditions
/// - `src` is a valid, non-null pointer to Flash/PROGMEM storage - NOT a RAM pointer;
///   providing one causes undefined behavior.
/// - On AVR, `src` must be obtainable from a variable declared with LISH_PROGMEM.
/// - The pointed-to object is fully initialized.
///
/// @tparam T The type to read. Should be a trivially copyable type.
///
/// @param src Pointer to a value in Flash/PROGMEM storage. On AVR, this must
///            be a pointer obtained from a variable declared with LISH_PROGMEM.
///            On other platforms, may point to any const data.
///
/// @return A copy of the value at `src`, read safely from Flash storage.
///
/// @note On AVR, this function performs a safe read from Harvard-architecture
///       Flash memory. On other platforms, this is equivalent to dereferencing.
///
/// @warning Undefined behavior if `src` points to RAM instead of Flash/PROGMEM.
///          Providing an invalid pointer results in unpredictable behavior.
template <typename T>
inline T read_flash(const T* const src) {
#if defined(__AVR__)
  T result;
  memcpy_P(&result, src, sizeof(T));
  return result;
#else
  return *src;
#endif
}

/// @brief Reads a single byte/character from a Flash string pointer.
///
/// @details
/// On AVR (Harvard architecture), reads a single byte from Flash memory using
/// pgm_read_byte(). On ARM and unified address space platforms, performs a
/// direct pointer dereference since Flash and RAM share the same address space.
///
/// This function is optimized for reading individual characters from Flash
/// strings, especially when iterating through string data.
///
/// ## Preconditions
/// - `ptr` is a valid, non-null pointer to Flash/PROGMEM storage - NOT a RAM pointer;
///   providing one causes undefined behavior.
/// - On AVR, `ptr` must be obtainable from a variable or string declared with
///   LISH_PROGMEM.
/// - The byte at `ptr` is fully initialized.
///
/// @param ptr Pointer to a character in Flash/PROGMEM storage. On AVR, this must
///            be a pointer obtained from a variable or string declared with
///            LISH_PROGMEM. On other platforms, may point to any const data.
///
/// @return The byte/character at `ptr`, read safely from Flash storage.
///
/// @note On AVR, this function performs a single-byte read from Harvard-architecture
///       Flash memory. On other platforms, this is equivalent to dereferencing.
///
/// @warning Undefined behavior if `ptr` points to RAM instead of Flash/PROGMEM.
///          Providing an invalid pointer results in unpredictable behavior.
///
/// @see read_flash() for reading larger data types from Flash
/// @see flash_string_length() for safely measuring Flash strings
inline char read_flash_char(const char* const ptr) {
#if defined(__AVR__)
  return static_cast<char>(pgm_read_byte(ptr));
#else
  return *ptr;
#endif
}

/// @brief Compares two strings case-insensitively, with Flash memory support.
///
/// @details
/// Compares a RAM string against a Flash/PROGMEM string with case-insensitive matching.
/// On AVR, reads Flash bytes via read_flash_char(). On ARM/unified, performs direct comparison.
///
/// ## Preconditions
/// - `ram_str` MUST point to valid RAM data. It is ALWAYS accessed by direct
///   dereference (`ram_str[pos]`), regardless of `use_flash_read` - passing a Flash
///   pointer here is undefined behavior on AVR even if `use_flash_read` is false,
///   since that flag only controls how `flash_str` is read.
/// - `flash_str` MUST point to valid Flash/PROGMEM data if `use_flash_read` is true;
///   otherwise it is also accessed by direct dereference, so it must then point to
///   valid RAM data instead (despite the parameter's name).
/// - Both strings must be null-terminated.
///
/// @param ram_str Pointer to null-terminated string in RAM. Always direct-dereferenced.
/// @param flash_str Pointer to null-terminated string in Flash if use_flash_read is
///                  true, otherwise in RAM (despite the name - see Preconditions).
/// @param use_flash_read If true, uses read_flash_char() for flash_str; if false, direct dereference.
///
/// @return true if strings match (case-insensitive), false otherwise.
///
/// @warning If use_flash_read is true, flash_str MUST point to Flash/PROGMEM data,
///          otherwise undefined behavior. If use_flash_read is false, BOTH parameters
///          are read as RAM - never pass a Flash pointer for either one in that case
///          (see lishTabComplete.h's fixed call for the mistake this warns against).
inline bool string_matches(const char* const ram_str, const char* const flash_str, const bool use_flash_read) {
  size_t pos = 0;
  while (true) {
    char flash_char = use_flash_read ? read_flash_char(flash_str + pos) : flash_str[pos];
    char ram_char = ram_str[pos];

    if (flash_char == '\0' || ram_char == '\0') {
      return flash_char == ram_char;
    }

    if (to_lower(ram_char) != to_lower(flash_char)) {
      return false;
    }
    ++pos;
  }
}

/// @brief Writes a RAM string to IO adapter.
///
/// @tparam IOAdapter Type with write(char) method.
///
/// @param str Pointer to a null-terminated string in RAM. If null, this is a no-op.
/// @param io IO adapter instance for writing output.
template<typename IOAdapter>
inline void write_string(const char* const str, IOAdapter& io) {
  if (!str) {
    return;
  }
  for (size_t i = 0; str[i] != '\0'; ++i) {
    io.write(str[i]);
  }
}

/// @brief Writes a Flash string to IO adapter.
///
/// @details
/// Outputs a null-terminated string stored in Flash/PROGMEM to an IO adapter.
/// On AVR, uses read_flash_char() for safe byte-by-byte access. On ARM/unified,
/// performs direct memory access.
///
/// ## Preconditions
/// - `flash_str` is null, or points to valid, properly null-terminated data in
///   Flash/PROGMEM storage.
///
/// @tparam IOAdapter Type with write(char) method.
///
/// @param flash_str Pointer to null-terminated string in Flash/PROGMEM storage.
/// @param io IO adapter instance for writing output.
///
/// @warning flash_str MUST point to Flash/PROGMEM data; providing RAM pointer
///          may produce incorrect output or undefined behavior on AVR.
template<typename IOAdapter>
inline void write_flash_string(const char* const flash_str, IOAdapter& io) {
  if (!flash_str) {
    return;
  }
  for (size_t i = 0; ; ++i) {
    char c = read_flash_char(flash_str + i);
    if (c == '\0') {
      break;
    }
    io.write(c);
  }
}

/// @brief Measures length of a null-terminated string in Flash storage.
///
/// @details
/// Safely measures the length of a null-terminated string stored in Flash memory.
/// On AVR (Harvard architecture), iterates using read_flash_char() to safely access
/// each byte. On ARM and unified address space platforms, delegates to standard
/// strlen() for efficiency.
///
/// ## Preconditions
/// - `str` is a valid, non-null pointer to Flash/PROGMEM storage - NOT a RAM pointer;
///   providing one causes undefined behavior.
/// - On AVR, `str` must be obtainable from a variable or string literal declared
///   with LISH_PROGMEM.
/// - The string at `str` is properly null-terminated and does not exceed 255 bytes
///   (uint8_t return type limit).
///
/// @param str Pointer to a null-terminated string in Flash/PROGMEM storage.
///            On AVR, this must be a pointer obtained from a variable or string
///            literal declared with LISH_PROGMEM. On other platforms, may point
///            to any const string data.
///
/// @return Length of the string in bytes, excluding the null terminator.
///         Maximum value is 255 (uint8_t limit).
///
/// @note On AVR, this function safely iterates through Flash memory using byte-by-byte
///       reads. On other platforms, this is equivalent to calling strlen().
///
/// @warning Undefined behavior if `str` points to RAM instead of Flash/PROGMEM.
///          Providing an invalid pointer or a non-null-terminated string results
///          in unpredictable behavior. Strings longer than 255 bytes will return
///          an incorrect (truncated) length.
///
/// @see read_flash_char() for reading individual bytes from Flash
/// @see read_flash() for reading larger data types from Flash
inline uint8_t flash_string_length(const char* const str) {
#if defined(__AVR__)
  uint8_t len = 0;
  while (read_flash_char(str + len) != '\0') {
    len++;
  }
  return len;
#else
  return static_cast<uint8_t>(strlen(str));
#endif
}

} // namespace lish

#endif // LISH_PLATFORM_H