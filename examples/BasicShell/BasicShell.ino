/*
  BasicShell - Interactive Command Shell Example for Arduino

  This example demonstrates a minimal lish interactive shell running on an Arduino
  with commands accessible over the Serial port. Users can type commands, use arrow
  keys for history navigation, tab completion, and more.

  Commands in this example:
  - help          Show available commands
  - echo [text]   Echo text back
  - uptime        Report system uptime
  - mode [0-6]    Get or set permission mode (Mode1-6; not available in Mode0)
  - login         Log in and elevate to Mode1 or Mode2 (only available in Mode0)
  - logout        Log out and return to Mode0 (not available in Mode0)
  - whoami        Show the currently logged-in user
  - sudo <cmd>    Run a command temporarily elevated to Mode6

  Hardware: Any Arduino with UART (tested on Arduino Uno, Nano, Mega)
  Serial Speed: 9600 baud

  Wiring: None required (uses built-in Serial)
*/

#include <lish.h>
#include <stdio.h>
#include "shell_config.h"

// ============================================================================
// Serial I/O Adapter for Arduino
// ============================================================================

class SerialIOAdapter {
public:
  void write(const char c) {
    Serial.write(c);
  }

  /// @brief Read a character with optional timeout.
  /// @param c Reference to store the read character
  /// @param timeout_ms Timeout in milliseconds:
  ///   - 0 (default): non-blocking, return false if no input available
  ///   - 0xFFFF: block forever until character arrives
  ///   - N: block for up to N milliseconds
  /// @return true if character was read, false on timeout or no input
  bool read(char& c, const uint16_t timeout_ms = 0) {
    unsigned long start = millis();
    bool infinite = (timeout_ms == 0xFFFF);

    while (true) {
      if (Serial.available() > 0) {
        c = Serial.read();
        return true;
      }
      if (!infinite && (millis() - start) >= timeout_ms) {
        return false;
      }
    }
  }
};

// ============================================================================
// Custom Prompt Callback (Optional)
// ============================================================================

LISH_FLASH_STORAGE char PROMPT_PREFIX[] LISH_PROGMEM = "[";
LISH_FLASH_STORAGE char PROMPT_SUFFIX[] LISH_PROGMEM = "] > ";

/// @brief Custom prompt callback that has access to the shell and context.
/// @param shell Reference to the IShell interface
/// @param ctx Reference to the application context (type-safe, no casting needed)
void custom_prompt(lish::IShell& shell, AppContext& ctx) {
  // Prints "<user>[<mode>] > ", e.g. "unpriv[1] > " - the current user (empty
  // if not logged in) followed by the current mode number, using flash strings
  // for the surrounding literal text.
  const uint8_t mode_num = __builtin_ctz(static_cast<uint8_t>(shell.mode()));
  char modeStr[4]{};
  snprintf(modeStr, 4, "%u", mode_num);
  lish::write_string(ctx.current_user, shell);
  lish::write_flash_string(PROMPT_PREFIX, shell);
  lish::write_string(modeStr, shell);
  lish::write_flash_string(PROMPT_SUFFIX, shell);
}

// ============================================================================
// Banner Strings (Flash)
// ============================================================================

// "LiSh" logo: "Li" in bright white on a (dim/standard-intensity) red
// background, "Sh" in black on a bright white background - VT100/ANSI SGR
// codes 41 (red bg), 97 (bright white fg), 107 (bright white bg), 30
// (black fg), reset with 0 before the rest of the banner text so the
// styling doesn't bleed into it.
LISH_FLASH_STORAGE char BANNER_1[] LISH_PROGMEM = "\n\r\x1B[41;97mLi\x1B[107;30mSh\x1B[0m - Lightweight Shell\n\r";
LISH_FLASH_STORAGE char BANNER_2[] LISH_PROGMEM = "Type 'help' for available commands\n\r";

// ============================================================================
// Global Objects
// ============================================================================

/// @brief Context object, passed by reference to each command, providing a way for shell
/// apps to maintain context - for example, can be used to store present working directory
/// if you have a file system, or any state you want preserved between calls to commands.
/// Each shell instance can have its own.
/// Note, for changing system state, commands can still call system level APIs.
AppContext app_ctx = { { 0 } };

// Shell instance created with the make_shell helper.
// Explicit template parameters here: (IOAdapterType, HistoryLength, MaxLineLength).
// A 4th, TypicalLineLength (history buffer sizing hint, default 10), can also be
// specified explicitly if needed. ContextType and NumCommands are always deduced -
// the latter from the commands array's own size (see shell_config.h).
// The helper also handles LineBuffer, LineEditor, History, and IOAdapter creation;
// the shell owns the IOAdapter, ensuring exclusive access to the I/O channel.
//
// The third argument, &custom_prompt, is optional (omit it, or pass nullptr, to use
// the library's default "> " prompt instead). The callback's signature must match
// PromptCallback = void (*)(IShell&, ContextType&) - here, void (*)(IShell&, AppContext&),
// matching command handlers' own (Args&, IShell&, ContextType&) signature in spirit
// (both take the real, typed ContextType&, never a type-erased void*).
static auto g_shell = lish::make_shell<SerialIOAdapter, 5, 32>(
  app_ctx,
  commands,
  &custom_prompt);


// ============================================================================
// Arduino Setup & Loop
// ============================================================================

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  delay(100);  // Wait for serial to stabilize

  // Print banner from flash
  lish::write_flash_string(BANNER_1, g_shell);
  lish::write_flash_string(BANNER_2, g_shell);

  // Print prompt (custom or default) on first shell service
  g_shell.print_prompt();
}

void loop() {
  // Service the shell - drains and processes all input currently available,
  // then returns; call repeatedly (e.g. every loop() iteration) rather than once.
  // NOTE: service() itself does not block, but a command it dispatches can,
  // especially one reading further input from the user (e.g. "login").
  g_shell.service();

  // Could add other tasks here
  // Example: update sensors, toggle LEDs, etc.
}
