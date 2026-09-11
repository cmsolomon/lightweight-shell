<p align="center">
  <img src="assets/lish.svg" alt="LiSh" width="80">
</p>

# Getting Started

This walks through everything needed to wire LiSh into your own sketch: the
I/O adapter, writing a command, the command array, and the context object.
Every code sample here is drawn from the real, compiling
[`examples/BasicShell`](../examples/BasicShell) - see that folder for a
complete, runnable sketch with multiple commands, login/permission modes,
and a custom prompt.

## 1. The I/O Adapter

LiSh talks to the outside world through an I/O adapter: any type with two
methods.

```cpp
class SerialIOAdapter {
public:
  void write(char c) {
    Serial.write(c);
  }

  bool read(char& c, uint16_t timeout_ms = 0) {
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
```

There's no base class to inherit from - `write`/`read` are called directly
through a template parameter, not through a virtual interface, so any type
with matching signatures works. `timeout_ms` has three meanings your `read`
must honor:

- `0` (the default) - non-blocking: return `false` immediately if nothing is available
- `0xFFFF` - block forever until a character arrives
- any other value `N` - block for up to `N` milliseconds, then return `false`

This is exactly the contract `Shell` itself relies on: its own `service()`
loop calls `read()` with no explicit timeout (the `0` default) to drain
whatever's currently available without blocking the rest of your `loop()`.

Because the adapter is a plain type, it isn't limited to `Serial` - the same
shape works for a Bluetooth SPP stream, USB CDC, a TCP socket, or an
in-memory buffer for tests.

## 2. Writing a Command

A command is a free function with a fixed signature, plus a `CmdDescriptor`
that describes it:

```cpp
// cmd_echo.cpp
LISH_FLASH_STORAGE char CMD_ECHO_NAME[] LISH_PROGMEM = "echo";
LISH_FLASH_STORAGE char CMD_ECHO_HELP[] LISH_PROGMEM = "Echo arguments back to output";

int8_t cmd_echo(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(ctx);  // This command doesn't need the context
  for (uint8_t i = 0; i < args.count(); ++i) {
    lish::write_string(args[i], shell);
    shell.write(' ');
  }
  lish::write_flash_string(lish::NEWLINE, shell);
  return 0;
}

LISH_FLASH_STORAGE lish::CmdDescriptor LISH_PROGMEM cmd_echo_descriptor =
  lish::CmdDescriptor::make<AppContext, &cmd_echo>(
    CMD_ECHO_NAME, lish::CmdPermission::Mode6, CMD_ECHO_HELP);
```

A handler always has this shape:

```cpp
int8_t handler(const lish::Args& args, lish::IShell& shell, YourContextType& ctx);
```

- **`args`** - the parsed argument list for this invocation. `args.count()`
  is the argument count (not including the command name itself);
  `args[i]` returns argument `i` as a `const char*`, or `nullptr` if
  `i >= args.count()`.
- **`shell`** - the `IShell&` for this dispatch. Use `shell.write(c)` for a
  single character, or the `lish::write_string()` /
  `lish::write_flash_string()` helpers (from `lishPlatform.h`) for whole
  strings from RAM or Flash respectively. A command can also call
  `shell.run_line(...)` reentrantly - see `examples/BasicShell/cmd_sudo.cpp`
  for a command that re-dispatches another line at an elevated permission.
- **`ctx`** - your application context, by reference (see [§4](#4-the-context-object)
  below). If a command doesn't need it, mark it explicitly with
  `lish::unused(ctx)` rather than leaving it silently unused.
- **Return value** - an `int8_t`, entirely application-defined. `0`
  conventionally means success; any other value becomes the command's exit
  code, later visible via `IShell::get_last_result_code()` and in the
  shell's own printed status line (`[OK]` for a zero return, `[ERROR: Code
  NNN]` otherwise).

`CmdDescriptor::make<ContextType, &handler>(name, permission, help)` builds
the descriptor: `ContextType` must match your handler's context parameter
type exactly (every command sharing one `Shell` instance uses the same
`ContextType`), and `permission` is a `lish::CmdPermission` value (see
[§5](#5-permission-modes) below).

**`LISH_PROGMEM`/`LISH_FLASH_STORAGE` on the name string, the help string,
and the descriptor itself (as shown above) are not optional on AVR** - this
is a correctness requirement, not a size optimization. `CmdDescriptor`'s
accessors read all three back via Flash-load instructions
(`pgm_read_byte`/`memcpy_P`) unconditionally, regardless of where you
actually put them. AVR is Harvard-architecture: RAM and Flash are separate
address spaces that both start at address 0, so if you skip the macros and
let a string or descriptor land in RAM instead, a Flash-load instruction
given that RAM address doesn't fail - it reads whatever happens to be at
that same numeric address *in Flash*, silently producing garbled command
names and help text. It's a no-op on unified-address-space targets (ARM,
x86, this repo's own host tests), which is exactly why the bug is invisible
until you flash real AVR hardware - passing on Uno R4 or in the host test
suite proves nothing about Uno/Nano/Mega correctness here. See
[Memory & Flash Footprint](../README.md#memory--flash-footprint) for the
size upside, but treat the macros themselves as required on any AVR target.

## 3. The Command Array

Every command's descriptor goes into one array, referenced (not copied) by
the shell:

```cpp
// shell_config.h
#include "cmd_echo.h"
#include "cmd_uptime.h"
// ... one #include per command ...

LISH_FLASH_STORAGE lish::CmdDescriptor* const LISH_PROGMEM commands[] = {
  &cmd_echo_descriptor,
  &cmd_uptime_descriptor,
  // ...
};
```

An entry may be `nullptr` for an intentionally-unregistered slot (a command
temporarily disabled, for instance) - `Shell` skips `nullptr` entries during
lookup rather than treating them as an error. The array's size becomes
`NumCommands`, deduced automatically wherever it's passed to
`make_shell()` - you never state the count yourself. Note that the built-in
`help` command is *not* part of this array; it's handled specially and
needs no entry.

## 4. The Context Object

The context is your application's own state, passed by reference to every
command handler and to a custom prompt callback if you use one. It exists
so commands can read and modify shared application state without any
global variables:

```cpp
// app_context.h
struct AppContext {
  char current_user[16];  // Empty string if not logged in
};
```

There's nothing special about this type - it's plain data, shaped however
your application needs (a present-working-directory string, sensor
readings, connection state, whatever your commands need to share). One
instance is constructed by you and handed to `make_shell()`; `Shell` stores
a reference to it (via a type-erased pointer internally) and casts back to
your real `ContextType` when invoking a handler - so **the context object's
lifetime must exceed the `Shell`'s** (a `static`/global instance, as in the
example below, is the usual choice).

## 5. Permission Modes

`lish::CmdPermission` is a bitmask covering 7 independent "modes" (bits
0-6) your application defines the meaning of (e.g. locked / user / admin /
debug), plus a separate visibility flag (bit 7, `HideWhenUnavailable`):

```cpp
lish::CmdPermission::Mode0                          // available only in Mode0
lish::CmdPermission::Mode1 | lish::CmdPermission::Mode2   // available in Mode1 or Mode2
lish::CmdPermission::AllModes                        // available in every mode
lish::CmdPermission::Mode6 | lish::CmdPermission::HideWhenUnavailable  // Mode6-only, and
                                                      // omitted entirely from `help`
                                                      // listings elsewhere, rather than
                                                      // shown dimmed
```

The shell's own current mode is set via `shell.set_mode(...)` (from within
a command - see `examples/BasicShell/cmd_mode.cpp` and `cmd_login.cpp` for
a login flow that elevates it) and read via `shell.mode()`. Tab completion
and the built-in `help` command both automatically respect whichever mode
is currently active.

## 6. Putting It Together

```cpp
#include <lish.h>
#include "app_context.h"
#include "shell_config.h"  // defines `commands[]` - see §3 above

class SerialIOAdapter { /* ... see §1 ... */ };

AppContext app_ctx = { { 0 } };

// Template parameters: <IOAdapterType, HistoryLength, MaxLineLength>.
// ContextType and NumCommands are deduced - the latter from commands[]'s own size.
static auto g_shell = lish::make_shell<SerialIOAdapter, 5, 32>(app_ctx, commands);

void setup() {
  Serial.begin(9600);
  g_shell.print_prompt();
}

void loop() {
  g_shell.service();  // Drains and processes all currently-available input, then returns
}
```

`make_shell()` also accepts an optional third argument, a custom prompt
callback (`void (*)(lish::IShell&, ContextType&)`), and a fourth for the
starting permission mode (defaults to `Mode0`) - see
`examples/BasicShell/BasicShell.ino`'s `custom_prompt()` for a worked
example that prints the logged-in user and current mode.

`service()` is non-blocking and processes only what's currently available -
call it repeatedly (every `loop()` iteration), not once. A command handler
itself *can* block (e.g. one that reads further input from the user, like a
login prompt), but `service()` never blocks waiting for the *next* line.

## Next Steps

- Read through [`examples/BasicShell`](../examples/BasicShell) end to end -
  it demonstrates everything above plus multi-mode permissions, a login
  flow, and a `sudo`-style reentrant command.
- When you connect to try it out, use a real terminal program, not the
  Arduino IDE's built-in Serial Monitor - see
  [Terminal Requirements](../README.md#terminal-requirements) in the
  README for why and what to use instead.
- See the [Architecture](ARCHITECTURE.md) primer if you're contributing to
  LiSh itself, not just using it.
