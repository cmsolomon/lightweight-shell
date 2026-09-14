#!/bin/bash
# lish (lightweight-shell) Prerequisites Installation Script
#
# Installs everything needed to both BUILD and PROGRAM (flash) lish, on a
# clean install of the supported platforms:
# - Host-side BDD test toolchain: CMake, C++ compiler, build tools, Boost, Google
#   Test, nlohmann_json (cwt-cucumber's own dependencies)
# - The Arduino toolchain: arduino-cli itself, plus the board cores for every
#   preset CMakeLists.txt currently supports (arduino:avr for uno/mega,
#   arduino:renesas_uno for uno_r4/uno_r4_minima, arduino:esp32 for
#   nano_esp32, esp32:esp32 - Espressif's own core, not Arduino's - for
#   xiao_esp32c5) - required for `make arduino_build`/`make arduino_run`,
#   which fail at CMake-configure time (arduino-cli missing) or build time (a
#   core missing) without these
# - pyserial (Linux/macOS): both ESP32 cores' esptool_py tool is a plain
#   Python script, not a self-contained binary like the other cores' upload
#   tools - it needs the system python3 plus the `serial` module. Without
#   it, `arduino_build`/`arduino_run` for nano_esp32/xiao_esp32c5 fail with
#   "ModuleNotFoundError: No module named 'serial'" from inside arduino-cli's
#   own bundled esptool.py, which looks like a broken install rather than a
#   missing Python dependency.
# - Serial port permissions (the `dialout` group) needed to actually upload to a
#   board via `make arduino_run` - without this, uploads fail with a permission
#   error on /dev/ttyUSB0 or /dev/ttyACM0, one of the most common first-time
#   Arduino stumbling blocks
#
# NOTE: New Arduino board presets should be added to CMakeLists.txt's
# ARDUINO_BOARD_PRESET options FIRST, then mirrored here (a new REQUIRED_CORE,
# and - for a third-party core - a
# `arduino-cli config add board_manager.additional_urls <url>` step) - keep
# this script's supported-board list in sync with CMakeLists.txt's, not ahead
# of it.
#
# Usage: ./install-prerequisites.sh
#        ./install-prerequisites.sh --install-udev-rules
#
# --install-udev-rules (Linux only, opt-in - modifies system udev config, and
#   is skipped by default): installs /etc/udev/rules.d rules granting
#   permission to (a) open Arduino boards' serial ports directly by vendor ID
#   (redundant with dialout group membership on most distros, but explicit
#   here for robustness) and (b) the raw USB DFU device node some boards
#   (e.g. Nano ESP32) briefly re-enumerate as mid-upload - (b) is NOT covered
#   by dialout group membership at all, since it's not a tty device; without
#   it, `arduino_run` on such a board fails with
#   "dfu-util: Cannot open DFU device ... (LIBUSB_ERROR_ACCESS)".
#
# Supported platforms:
# - Ubuntu/Debian (apt)
# - Fedora/RHEL/CentOS (dnf/yum)
# - macOS (Homebrew)

set -e

INSTALL_UDEV_RULES=0
for arg in "$@"; do
    case "$arg" in
        --install-udev-rules)
            INSTALL_UDEV_RULES=1
            ;;
        *)
            echo "Unknown argument: $arg" >&2
            echo "Usage: $0 [--install-udev-rules]" >&2
            exit 1
            ;;
    esac
done

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${YELLOW}🧪 lish - Prerequisites Installation${NC}"
echo "=================================================="

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
    if command -v apt-get &> /dev/null; then
        PKG_MANAGER="apt"
    elif command -v dnf &> /dev/null; then
        PKG_MANAGER="yum"  # Use yum handler for dnf (compatible)
    elif command -v yum &> /dev/null; then
        PKG_MANAGER="yum"
    else
        echo -e "${RED}✗ Unsupported Linux distribution${NC}"
        exit 1
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
    PKG_MANAGER="brew"
else
    echo -e "${RED}✗ Unsupported OS: $OSTYPE${NC}"
    exit 1
fi

echo -e "${YELLOW}Detected OS: $OS (Package Manager: $PKG_MANAGER)${NC}"
echo ""

# Function to check if command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to check package installation
package_installed() {
    if [ "$PKG_MANAGER" = "apt" ]; then
        dpkg -l | grep -q "^ii  $1"
    elif [ "$PKG_MANAGER" = "yum" ]; then
        yum list installed "$1" &> /dev/null
    elif [ "$PKG_MANAGER" = "brew" ]; then
        brew list "$1" &> /dev/null
    fi
}

# ============================================================================
# Ubuntu/Debian Installation
# ============================================================================
if [ "$PKG_MANAGER" = "apt" ]; then
    echo -e "${YELLOW}Installing prerequisites for Ubuntu/Debian...${NC}"

    # Update package lists
    echo "Updating package lists..."
    sudo apt-get update -qq

    # Core build tools
    echo "Installing build tools..."
    sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        curl \
        wget \
        pkg-config

    # C++ libraries (Boost, nlohmann-json)
    echo "Installing C++ libraries..."
    sudo apt-get install -y \
        libboost-all-dev \
        libboost-test-dev \
        nlohmann-json3-dev \
        netcat-openbsd

    # Google Test
    echo "Installing Google Test..."
    sudo apt-get install -y \
        libgtest-dev

    # lcov (also provides genhtml) - coverage instrumentation is mandatory,
    # not optional (see CMakeLists.txt), so this is a hard requirement, not
    # a nice-to-have.
    echo "Installing lcov..."
    sudo apt-get install -y \
        lcov


# ============================================================================
# Fedora/RHEL/CentOS Installation (dnf/yum)
# ============================================================================
elif [ "$PKG_MANAGER" = "yum" ]; then
    echo -e "${YELLOW}Installing prerequisites for Fedora/RHEL/CentOS...${NC}"

    # Prefer dnf on newer systems
    if command_exists dnf; then
        PKG_CMD="dnf"
        SUDO_PKG="sudo dnf"
    else
        PKG_CMD="yum"
        SUDO_PKG="sudo yum"
    fi

    # Update package lists
    echo "Updating package lists..."
    $SUDO_PKG update -y -q

    # Core build tools
    echo "Installing build tools..."
    if [ "$PKG_CMD" = "dnf" ]; then
        $SUDO_PKG group install -y "Development Tools"
    else
        $SUDO_PKG groupinstall -y "Development Tools"
    fi
    $SUDO_PKG install -y \
        cmake \
        git \
        curl \
        wget \
        pkg-config

    # C++ libraries (Boost, nlohmann-json)
    echo "Installing C++ libraries..."
    $SUDO_PKG install -y \
        boost-devel \
        boost-system \
        boost-thread \
        nlohmann-json-devel \
        netcat

    # Google Test
    echo "Installing Google Test..."
    $SUDO_PKG install -y \
        gtest-devel

    # lcov (also provides genhtml) - coverage instrumentation is mandatory,
    # not optional (see CMakeLists.txt), so this is a hard requirement, not
    # a nice-to-have.
    echo "Installing lcov..."
    $SUDO_PKG install -y \
        lcov


# ============================================================================
# macOS Installation (Homebrew)
# ============================================================================
elif [ "$PKG_MANAGER" = "brew" ]; then
    echo -e "${YELLOW}Installing prerequisites for macOS...${NC}"

    # Install Homebrew if needed
    if ! command_exists brew; then
        echo "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi

    # Update Homebrew
    echo "Updating Homebrew..."
    brew update

    # Core build tools
    echo "Installing build tools..."
    brew install cmake git

    # C++ libraries
    echo "Installing C++ libraries..."
    brew install boost nlohmann-json

    # Google Test
    echo "Installing Google Test..."
    brew install googletest

    # lcov (also provides genhtml) - coverage instrumentation is mandatory,
    # not optional (see CMakeLists.txt), so this is a hard requirement, not
    # a nice-to-have.
    echo "Installing lcov..."
    brew install lcov

fi

# ============================================================================
# Arduino Toolchain (build + program the actual target board)
# ============================================================================
# Installed the same way on Linux and macOS: arduino-cli's own installer script
# detects OS/arch itself. CMakeLists.txt's `arduino_build`/`arduino_run` targets
# hard-require arduino-cli (a FATAL_ERROR at CMake-configure time if it's
# missing), and separately require whichever REQUIRED_CORE their selected
# ARDUINO_BOARD_PRESET maps to (a build-time failure from arduino-cli itself,
# not CMake, if that core isn't installed) - both are handled here upfront so
# `make arduino_build`/`make arduino_run` work the first time, not just the
# host-side test suite.
echo ""
echo -e "${YELLOW}Installing Arduino toolchain...${NC}"

ARDUINO_BIN_DIR="$HOME/.local/bin"

if command_exists arduino-cli; then
    echo -e "${GREEN}✓ arduino-cli already installed${NC}: $(arduino-cli version)"
else
    echo "Installing arduino-cli to $ARDUINO_BIN_DIR..."
    mkdir -p "$ARDUINO_BIN_DIR"
    # BINDIR overrides the installer's default of installing into ./bin (the
    # current directory) - without it, running this script from the repo root
    # would drop a bin/arduino-cli into the repo itself.
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | BINDIR="$ARDUINO_BIN_DIR" sh

    case ":$PATH:" in
        *":$ARDUINO_BIN_DIR:"*) ;;
        *)
            echo -e "${YELLOW}⚠ $ARDUINO_BIN_DIR is not on your PATH.${NC}"
            echo "  Add this to your shell profile (~/.bashrc, ~/.zshrc, etc.):"
            echo "    export PATH=\"\$PATH:$ARDUINO_BIN_DIR\""
            ;;
    esac

    # Use the freshly-installed binary directly for the rest of this script,
    # since the PATH update above won't take effect until a new shell.
    export PATH="$ARDUINO_BIN_DIR:$PATH"
fi

# arduino-lint validates library.properties/structure against the exact
# rules Arduino's Library Manager registry enforces - installed here too so
# a compliance issue is caught locally, before it's discovered as a failed
# release workflow run (see .github/workflows/release.yml).
if command_exists arduino-lint; then
    echo -e "${GREEN}✓ arduino-lint already installed${NC}: $(arduino-lint --version)"
else
    echo "Installing arduino-lint to $ARDUINO_BIN_DIR..."
    mkdir -p "$ARDUINO_BIN_DIR"
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-lint/main/etc/install.sh | BINDIR="$ARDUINO_BIN_DIR" sh
    export PATH="$ARDUINO_BIN_DIR:$PATH"
fi

# esp32:esp32 (Espressif's own core, needed for xiao_esp32c5 - see below) is
# a third-party package, not part of arduino-cli's default/curated index -
# it needs its own Boards Manager URL added first, or `core install
# esp32:esp32` can't resolve the package at all. `config add` is idempotent
# (a repeat add of the same URL doesn't duplicate the list entry), so this
# is safe to run on every invocation of this script.
echo "Adding Espressif's ESP32 Boards Manager URL..."
arduino-cli config add board_manager.additional_urls \
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

# Refresh arduino-cli's package index before installing any core - required on
# a first run (there is no index to install from yet), and again here since
# the URL above just added a new source to fetch.
echo "Updating arduino-cli package index..."
arduino-cli core update-index

# Install the board core for every preset CMakeLists.txt currently supports.
# arduino:avr covers "uno"/"mega"; arduino:renesas_uno covers "uno_r4"/
# "uno_r4_minima"; arduino:esp32 covers "nano_esp32"; esp32:esp32 (a
# different, Espressif-maintained core - arduino:esp32 doesn't yet support
# the C5 chip) covers "xiao_esp32c5" - see CMakeLists.txt's
# ARDUINO_BOARD_PRESET options.
for core in "arduino:avr" "arduino:renesas_uno" "arduino:esp32" "esp32:esp32"; do
    if arduino-cli core list | grep -q "^${core} "; then
        echo -e "${GREEN}✓ $core core already installed${NC}"
    else
        echo "Installing $core core..."
        arduino-cli core install "$core"
    fi
done

# ============================================================================
# pyserial (Linux/macOS - required to upload/program an ESP32 board)
# ============================================================================
# Both arduino:esp32's and esp32:esp32's esptool_py tool are plain Python
# scripts relying on the system python3 plus the `serial` module (pyserial)
# - unlike the other cores' upload tools, they aren't self-contained
# binaries, so a missing pyserial doesn't surface until `arduino_run`
# actually tries to upload to a nano_esp32/xiao_esp32c5 board, as a
# ModuleNotFoundError from inside arduino-cli's own bundled esptool.py.
echo ""
echo -e "${YELLOW}Checking Python pyserial (required by the ESP32 cores' esptool)...${NC}"
if python3 -c "import serial" &> /dev/null; then
    echo -e "${GREEN}✓ pyserial already installed${NC}"
else
    echo "Installing pyserial..."
    if [ "$PKG_MANAGER" = "apt" ]; then
        sudo apt-get install -y python3-serial
    elif [ "$PKG_MANAGER" = "yum" ]; then
        $SUDO_PKG install -y python3-pyserial
    elif [ "$PKG_MANAGER" = "brew" ]; then
        python3 -m pip install --user pyserial
    fi
fi

# ============================================================================
# Serial Port Permissions (Linux only - required to upload/program a board)
# ============================================================================
# Uploading via `make arduino_run` opens /dev/ttyUSB0 or /dev/ttyACM0 directly;
# on a clean install a regular user isn't in the group that owns those device
# nodes, so the upload fails with a permission error. This is one of the most
# common first-time Arduino stumbling blocks, and easy to mistake for a real
# build/toolchain problem since the build itself succeeds.
if [ "$OS" = "linux" ]; then
    echo ""
    echo -e "${YELLOW}Checking serial port permissions...${NC}"

    # $USER isn't reliably set outside an interactive login shell (e.g. a CI
    # runner step, or `bash -c` invocations) - whoami always works.
    CURRENT_USER="$(whoami)"

    if groups "$CURRENT_USER" | grep -qw dialout; then
        echo -e "${GREEN}✓ $CURRENT_USER is already in the dialout group${NC}"
    else
        echo "Adding $CURRENT_USER to the dialout group..."
        sudo usermod -aG dialout "$CURRENT_USER"
        echo -e "${YELLOW}⚠ Group membership only takes effect on your next login.${NC}"
        echo "  Log out and back in (or run 'newgrp dialout' in this shell) before"
        echo "  attempting to upload to a board with 'make arduino_run'."
    fi
fi

# ============================================================================
# udev Rules (Linux only, opt-in via --install-udev-rules)
# ============================================================================
# Two separate problems, only one of which dialout group membership above
# actually fixes:
# - Serial (tty) access: normally already covered by dialout group
#   membership - this rule is redundant on most distros, included only for
#   robustness on minimal setups that don't grant dialout rw on tty nodes by
#   default.
# - Raw USB DFU access: some boards (e.g. Nano ESP32) briefly re-enumerate
#   as a raw USB DFU device mid-upload, NOT a tty - dialout group membership
#   doesn't apply to that device node at all. Without this rule, uploading
#   to such a board fails with "dfu-util: Cannot open DFU device ...
#   (LIBUSB_ERROR_ACCESS)" even with correct dialout membership, which looks
#   identical to a permissions problem the group-membership fix above should
#   have already solved.
# Both rules grant dialout group access only (0660/GROUP="dialout"), not
# world access (0666 would let any local user/process on the machine open
# or reprogram an attached Arduino board, not just this user) - matching
# the same group the dialout-membership fix above already relies on. The
# DFU rule matches by USB vendor ID only, not the specific DFU product
# ID(s), so it stays generic across whichever Arduino board actually needs
# it (different boards' DFU bootloaders can use different product IDs) -
# narrowing it further to just the Nano ESP32's PID would fix this one
# board at the cost of breaking it again for the next one.
# Skipped by default (not just by OS) since it writes to /etc/udev/rules.d
# and reloads the system udev daemon - system-wide, root-owned config that
# outlives this script and this user, so it's opt-in only.
if [ "$OS" = "linux" ] && [ "$INSTALL_UDEV_RULES" = "1" ]; then
    echo ""
    echo -e "${YELLOW}Installing udev rules for Arduino serial + DFU access...${NC}"

    SERIAL_RULES_FILE="/etc/udev/rules.d/99-arduino-serial.rules"
    DFU_RULES_FILE="/etc/udev/rules.d/99-arduino-dfu.rules"

    echo "Writing $SERIAL_RULES_FILE..."
    echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="2341", MODE="0660", GROUP="dialout"' | \
        sudo tee "$SERIAL_RULES_FILE" > /dev/null

    echo "Writing $DFU_RULES_FILE..."
    echo 'SUBSYSTEM=="usb", ATTR{idVendor}=="2341", MODE="0660", GROUP="dialout"' | \
        sudo tee "$DFU_RULES_FILE" > /dev/null

    echo "Reloading udev rules..."
    sudo udevadm control --reload-rules
    sudo udevadm trigger

    echo -e "${GREEN}✓ udev rules installed${NC}"
    echo -e "${YELLOW}⚠ Unplug and replug the board for the new rules to apply to it.${NC}"
fi

# ============================================================================
# Verification
# ============================================================================
echo ""
echo -e "${YELLOW}Verifying installation...${NC}"

missing=0

# Check CMake
if command_exists cmake; then
    echo -e "${GREEN}✓ CMake${NC}: $(cmake --version | head -1 | cut -d' ' -f3)"
else
    echo -e "${RED}✗ CMake not found${NC}"
    missing=$((missing + 1))
fi

# Check C++ compiler
if command_exists g++; then
    echo -e "${GREEN}✓ G++${NC}: $(g++ --version | head -1 | cut -d' ' -f3-)"
elif command_exists clang++; then
    echo -e "${GREEN}✓ Clang++${NC}: $(clang++ --version | head -1 | cut -d' ' -f3-)"
else
    echo -e "${RED}✗ C++ compiler not found${NC}"
    missing=$((missing + 1))
fi


# Check Boost
if [ "$OS" = "linux" ]; then
    if dpkg -l 2>/dev/null | grep -q "libboost"; then
        echo -e "${GREEN}✓ Boost libraries${NC} installed (APT)"
    elif rpm -q boost-devel &> /dev/null; then
        echo -e "${GREEN}✓ Boost libraries${NC} installed (RPM)"
    fi
elif [ "$OS" = "macos" ] && command_exists brew && brew list boost &> /dev/null; then
    echo -e "${GREEN}✓ Boost libraries${NC} installed"
fi

# Check lcov/genhtml (coverage is mandatory, not optional - see CMakeLists.txt)
if command_exists lcov; then
    echo -e "${GREEN}✓ lcov${NC}: $(lcov --version | head -1)"
else
    echo -e "${RED}✗ lcov not found${NC}"
    missing=$((missing + 1))
fi
if command_exists genhtml; then
    echo -e "${GREEN}✓ genhtml${NC} found"
else
    echo -e "${RED}✗ genhtml not found${NC}"
    missing=$((missing + 1))
fi


# Check arduino-cli and its board cores
if command_exists arduino-cli; then
    echo -e "${GREEN}✓ arduino-cli${NC}: $(arduino-cli version | head -1)"
    for core in "arduino:avr" "arduino:renesas_uno" "arduino:esp32" "esp32:esp32"; do
        if arduino-cli core list | grep -q "^${core} "; then
            echo -e "${GREEN}✓ $core core${NC} installed"
        else
            echo -e "${RED}✗ $core core not found${NC}"
            missing=$((missing + 1))
        fi
    done
else
    echo -e "${RED}✗ arduino-cli not found${NC}"
    missing=$((missing + 1))
fi

# Check arduino-lint
if command_exists arduino-lint; then
    echo -e "${GREEN}✓ arduino-lint${NC}: $(arduino-lint --version)"
else
    echo -e "${RED}✗ arduino-lint not found${NC}"
    missing=$((missing + 1))
fi

# Check pyserial (required by the ESP32 cores' esptool)
if python3 -c "import serial" &> /dev/null; then
    echo -e "${GREEN}✓ pyserial${NC} installed"
else
    echo -e "${RED}✗ pyserial not found${NC} (nano_esp32/xiao_esp32c5 uploads will fail until this is fixed)"
    missing=$((missing + 1))
fi

# Check serial port group membership (Linux only)
if [ "$OS" = "linux" ]; then
    CURRENT_USER="$(whoami)"
    if groups "$CURRENT_USER" | grep -qw dialout; then
        echo -e "${GREEN}✓ $CURRENT_USER in dialout group${NC} (upload permissions OK after next login)"
    else
        echo -e "${RED}✗ $CURRENT_USER not in dialout group${NC} (uploads will fail until this is fixed)"
        missing=$((missing + 1))
    fi
fi

echo ""
echo "=================================================="

if [ $missing -eq 0 ]; then
    echo -e "${GREEN}✓ All prerequisites installed!${NC}"
    echo ""
    echo "Next steps:"
    echo "  Host-side BDD tests:"
    echo "    1. cmake -S . -B build"
    echo "    2. cmake --build build"
    echo "    3. ctest --test-dir build --output-on-failure"
    echo ""
    echo "  Build and upload to a board:"
    echo "    4. cmake --build build --target arduino_build"
    echo "    5. cmake --build build --target arduino_run"
    echo ""
    echo "  If you were just added to the dialout group, log out and back in"
    echo "  (or run 'newgrp dialout') before step 5."
    echo ""
    echo "  Uploading to a board that fails with a USB/DFU permission error (e.g."
    echo "  'Cannot open DFU device ... LIBUSB_ERROR_ACCESS' on Nano ESP32), even"
    echo "  with dialout group membership already fixed above:"
    echo "    ./install-prerequisites.sh --install-udev-rules"
    exit 0
else
    echo -e "${RED}✗ $missing prerequisite(s) missing${NC}"
    echo ""
    echo "Please install the missing item(s) above manually, or re-run this script -"
    echo "run it as your normal user, not with sudo (it calls sudo itself only for"
    echo "the specific commands that need it; running the whole script as root would"
    echo "install arduino-cli and check group membership for root instead of you):"
    echo "  ./install-prerequisites.sh"
    exit 1
fi
