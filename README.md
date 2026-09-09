<p align="center">
  <img src="docs/assets/lish-horizontal.svg" alt="lish" width="360">
</p>

<p align="center">
  <strong>A zero-heap, no-vtable interactive command shell for Arduino and embedded C++.</strong>
</p>

<p align="center">
  <a href="LICENSE"><img alt="License: Apache-2.0" src="https://img.shields.io/badge/license-Apache--2.0-blue.svg"></a>
  <!-- TODO: GitHub Sponsors badge - pending sponsor account setup -->
</p>

---

## What is lish?

**lish** (short for *lightweight-shell*) gives a microcontroller a real, editable
command line over any character stream you can `write()` to and `read()` from:
arrow-key history, tab completion, backspace/cursor editing, multi-command
chaining (`;` / `&&`), quoted arguments, and role-based permission modes -
without heap allocation, and without virtual-call overhead for command
dispatch (each command is matched to its handler at compile time, through a
template-generated trampoline, not a vtable).

It's distributed as an **Arduino library** - discoverable via the Arduino
Library Manager - but the shell core itself has **no Arduino dependency at
all**. It only needs an I/O adapter type with two methods, `write(char)` and
`read(char&, uint16_t timeout_ms = 0)`. That makes it equally usable on any
embedded (or hosted) C++ target you can write such an adapter for: a UART,
USB CDC, Bluetooth SPP, a TCP socket, or even an in-memory buffer for tests.

## Features

- **Interactive line editing** - insertion, backspace, delete, cursor
  movement (Home/End/Left/Right), driven by real VT100/ANSI escape sequences
- **Command history** with up/down arrow navigation, backed by a fixed-size
  ring buffer
- **Tab completion**, filtered by the current permission mode
- **Command chaining** on one line: `cmd1 ; cmd2` (always run both) and
  `cmd1 && cmd2` (short-circuit on failure)
- **Quoted arguments** (`"..."` / `'...'`) with embedded spaces
- **Role-based access control** - 7 independent permission "modes" per
  command, plus an optional "hide entirely when unavailable" flag distinct
  from "show, dimmed"
- **Built-in `help` command** - lists available commands, or shows one
  command's help text, respecting the current mode's visibility rules, with
  zero extra registration
- **Zero heap allocation** - every buffer (line, history, ...) is a
  fixed-size array, sized by template parameters
- **Zero vtable overhead for dispatch** - only `IShell` itself is virtual
  (implemented once, by `Shell`); each command handler is wrapped in a
  compile-time trampoline, not a runtime lookup
- **Flash/PROGMEM-resident command metadata on AVR** - names, help text, and
  the descriptor table itself live in Flash, not RAM, via a platform
  abstraction that's a no-op on unified-address-space targets (ARM, x86, etc.)
- **Reentrant non-interactive dispatch** (`run_line()`) alongside the
  interactive path, so a command can safely re-invoke the shell - e.g. a
  `sudo <cmd>` command that re-dispatches at an elevated mode (see
  `examples/BasicShell/cmd_sudo.cpp`)

## Compatibility

- Distributed as an Arduino library (`library.properties`), installable via
  the Arduino Library Manager
- **Not Arduino-specific**: the shell core depends on nothing from the
  Arduino framework. Anything providing an I/O adapter
  (`write(char)` / `read(char&, uint16_t timeout_ms = 0)`) can host it
- Verified boards (via this repo's own build):
  Arduino Uno, Arduino Mega 2560 (AVR / 8-bit),
  Arduino Uno R4 & Uno R4 Minima (Renesas/ARM)
- Two Flash-storage strategies, selected automatically: AVR's
  Harvard-architecture PROGMEM (`avr/pgmspace.h`), or a plain-pointer path
  for unified-address-space targets (ARM and similar) - override either via
  the `LISH_PROGMEM` / `LISH_FLASH_STORAGE` macros if your target needs
  something else
- Targets C++11 language features throughout the library itself (no
  C++14/17/20-only constructs) - compiles under whatever C++11-era
  `avr-g++` your board's Arduino core ships. (Separately, this project's own
  host-side BDD test suite requires C++20 - a testing-tool requirement, not
  something your sketch needs.)

## Memory & Flash Footprint

Measured on an Arduino Uno (ATmega328P), via `arduino-cli`, the same
toolchain this repo's own build uses:

| Configuration | Flash | RAM |
|---|---:|---:|
| Empty sketch (baseline) | 444 B | 9 B |
| Serial only, no lish | 1,468 B | 184 B |
| lish wired to Serial, zero commands registered | 9,590 B | 342 B |
| `examples/BasicShell` (8 commands, login + permission modes) | 12,856 B | 415 B |

"lish wired to Serial, zero commands" is the smallest a real, working shell
built with this library can be. Serial alone (no lish at all) accounts for
only about 1 KB of that - the rest (roughly 8 KB) is genuinely the shell:
line editor, history, tab completion, dispatch, help formatting, ANSI
handling. We don't further split "the shell" from "your I/O driver" by
swapping in a no-op adapter - doing so lets the compiler prove entire
output-formatting code paths have no observable effect and discard them,
which understates the real cost rather than isolating it honestly.

Your own commands and `AppContext` add on top from there: the 8 commands in
`examples/BasicShell` (echo, uptime, mode, login, logout, whoami, sudo, plus
the built-in help) added roughly 3.3 KB over the zero-command baseline above
- actual cost per command varies with what it does.

## Quick Example

```cpp
#include <lish.h>

struct AppContext {};
AppContext app_ctx;

class SerialIOAdapter {
public:
  void write(char c) { Serial.write(c); }
  bool read(char& c, uint16_t timeout_ms = 0) {
    if (Serial.available()) { c = Serial.read(); return true; }
    return false;
  }
};

int8_t cmd_ping(const lish::Args& args, lish::IShell& shell, AppContext& ctx) {
  lish::unused(args);
  lish::unused(ctx);
  lish::write_string("pong\n", shell);
  return 0;
}

const lish::CmdDescriptor cmd_ping_descriptor = lish::CmdDescriptor::make<AppContext, &cmd_ping>(
  "ping", lish::CmdPermission::AllModes, "Replies with pong");

const lish::CmdDescriptor* const commands[] = { &cmd_ping_descriptor };

auto shell = lish::make_shell<SerialIOAdapter, 5, 32>(app_ctx, commands);

void setup() {
  Serial.begin(9600);
  shell.print_prompt();
}

void loop() {
  shell.service();
}
```

This version skips Flash/PROGMEM placement to stay minimal - see the
[Getting Started guide](docs/GETTING_STARTED.md) for the Flash-optimized
pattern `examples/BasicShell` actually uses.

## Terminal Requirements

The Arduino IDE's built-in Serial Monitor is not a good way to use lish -
use a dedicated terminal program instead. lish's interactive features -
line editing, cursor movement, arrow-key history, dimmed help listings -
all work by sending real VT100/ANSI escape sequences over the wire (see
[Features](#features) above). The built-in Serial Monitor is just a raw
text pane, not a terminal emulator, so it doesn't interpret any of this -
escape sequences show up as literal garbage characters instead of doing
anything.

Use a real terminal program instead:

- **[PuTTY](https://www.putty.org/)** (serial connection type) - the usual
  recommendation on Windows, but packaged for Linux too (e.g.
  `apt install putty`) and available on macOS via Homebrew - a solid choice
  on any platform
- **Linux/macOS**: `screen /dev/ttyUSB0 9600`, `picocom -b 9600 /dev/ttyUSB0`,
  or `minicom` - usually already installed, or a one-line package install,
  if you'd rather not add PuTTY
- **Windows**: Tera Term is a common PuTTY alternative
- **Any platform**: a serial-terminal editor extension (e.g. VS Code's
  "Serial Monitor" extension) works too, as long as it does real terminal
  emulation rather than just displaying raw bytes

## Documentation

- **[Getting Started](docs/GETTING_STARTED.md)** - writing an I/O adapter,
  writing commands, the command array, and what the context object is for
- **[examples/BasicShell](examples/BasicShell)** - a complete, runnable
  example sketch with multiple commands, login/permission modes, and a
  custom prompt
- **[Architecture](docs/ARCHITECTURE.md)** - an internals primer for
  contributors; not needed to use the library

## Contributing

Building lish itself and running its BDD test suite is covered separately
from this end-user documentation - see [`docs/SETUP.md`](docs/SETUP.md).

## License

[Apache License 2.0](LICENSE)
