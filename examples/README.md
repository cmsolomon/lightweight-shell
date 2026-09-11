# LiSh Examples

This directory contains example sketches demonstrating how to use the LiSh (lightweight-shell) embedded shell library.

## BasicShell

A minimal interactive shell for Arduino running over Serial (UART).

**Features demonstrated:**
- Simple IOAdapter implementation for Serial I/O
- Application context for tracking state
- Multiple command implementations, split across per-command .h/.cpp file pairs
- Interactive line editing with history and tab completion
- Role-based access control: permission modes, login/logout, and sudo-style privilege elevation

**Supported boards:** Any Arduino with UART (Uno, Nano, Mega, etc.)

**Getting started:**
1. Open `BasicShell.ino` in the Arduino IDE
2. Select your board and COM port
3. Upload the sketch
4. Open the Serial Monitor (9600 baud)
5. Type `help` to see available commands

**Commands:**
- `help` - List available commands
- `echo [text]` - Echo text back
- `uptime` - Report system uptime
- `mode [0-6]` - Get or set permission mode (Mode1-6; not available in Mode0)
- `login` - Log in and elevate to Mode1 or Mode2 (only available in Mode0)
- `logout` - Log out and return to Mode0 (not available in Mode0)
- `whoami` - Show the currently logged-in user
- `sudo <cmd>` - Run a command temporarily elevated to Mode6

**Interactive features:**
- Arrow keys for history navigation
- Backspace / Delete for editing
- Tab for command completion
- Ctrl+H and Ctrl+? for backspace
- Cursor movement (Home, End)

## Architecture Notes

The example shows the typical pattern for using LiSh:

```cpp
// 1. Define your application context - whatever state your commands need to
// share (current user, working directory, sensor readings, etc.)
struct AppContext {
  // Your application state goes here
};

// 2. Create an IOAdapter for your communication channel
class MyIOAdapter {
public:
  void write(const char c) { /* send to your channel */ }

  // timeout_ms: 0 = non-blocking (return false if nothing available),
  // 0xFFFF = block forever, N = block for up to N milliseconds
  bool read(char& c, const uint16_t timeout_ms = 0) { /* receive from your channel */ }
};

// 3. Implement command functions - context arrives as a real, typed reference,
// not something you fetch from the shell
int8_t my_command(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  // Output: shell.write('x') or lish::write_string("msg", shell)
  // Input: shell.read(c)
  // Check/change permissions: shell.mode() / shell.set_mode(...)
  return 0;  // exit code - 0 means success; any nonzero code (positive or
             // negative) means the command did not fully succeed
}

// 4. Declare a command descriptor for each command, then collect pointers to
// them into a command table. Handler and context type are bound at compile
// time via the make<>() template arguments.
//
// LISH_PROGMEM/LISH_FLASH_STORAGE below aren't optional on AVR: the name
// string, help string, descriptor, and command array are all read back
// through Flash-load instructions unconditionally, so leaving any of them
// in plain RAM doesn't just cost more RAM - it silently corrupts what gets
// read back (see docs/GETTING_STARTED.md, §2, for why).
LISH_FLASH_STORAGE char CMD_MY_COMMAND_NAME[] LISH_PROGMEM = "cmd";
LISH_FLASH_STORAGE char CMD_MY_COMMAND_HELP[] LISH_PROGMEM = "Help text";

LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_my_command_descriptor =
  lish::CmdDescriptor::make<AppContext, &my_command>(
    CMD_MY_COMMAND_NAME, lish::CmdPermission::Mode0, CMD_MY_COMMAND_HELP);

LISH_FLASH_STORAGE lish::CmdDescriptor* const LISH_PROGMEM commands[] = {
  &cmd_my_command_descriptor,
};

// 5. Instantiate the Shell with the make_shell() helper - it deduces
// NumCommands from the commands array, and constructs the LineBuffer,
// LineEditor, History, and IOAdapter for you (the Shell owns all of them).
AppContext ctx;
auto shell = lish::make_shell<MyIOAdapter, /*HistoryLength=*/16, /*MaxLineLength=*/64>(
  ctx, commands);

// 6. Call service() in your main loop
while (true) {
  // Drains and dispatches all input currently available, then returns -
  // non-blocking itself, though a command it dispatches may block (e.g. one
  // reading further input from the user, like "login" does).
  shell.service();
}
```

## Tips

- **Output:** Use `write_string()` for RAM strings and `write_flash_string()` for PROGMEM strings (saves RAM on AVR)
- **Context:** Store application state in your context struct for commands to access
- **IOAdapter:** Implement for any communication channel (Serial, I2C, SPI bridge, Bluetooth, etc.)
- **Commands:** Return 0 for success; any nonzero code - positive or negative - means the shell will report the command as failed, with that code, in its `[ERROR: Code NNN]` status line
- **Permissions:** Use CmdPermission modes to restrict commands to certain operational states

## Further Reading

- See `REQUIREMENTS.md` in the library root for detailed design documentation
- See `src/lish.h` for the main API reference
- See `CLAUDE.md` for code style guidelines
