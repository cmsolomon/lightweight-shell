#!/bin/bash
# lish (lightweight-shell) Prerequisites Installation Script
#
# Installs everything needed to both BUILD and PROGRAM (flash) lish, on a
# clean install of the supported platforms:
# - Host-side BDD test toolchain: CMake, C++ compiler, build tools, Boost, Google
#   Test, nlohmann_json (cwt-cucumber's own dependencies)
# - The Arduino toolchain: arduino-cli itself, plus the board cores for every
#   preset CMakeLists.txt currently supports (arduino:avr for uno/mega,
#   arduino:renesas_uno for uno_r4/uno_r4_minima) - required for `make
#   arduino_build`/`make arduino_run`, which fail at CMake-configure time
#   (arduino-cli missing) or build time (a core missing) without these
# - Serial port permissions (the `dialout` group) needed to actually upload to a
#   board via `make arduino_run` - without this, uploads fail with a permission
#   error on /dev/ttyUSB0 or /dev/ttyACM0, one of the most common first-time
#   Arduino stumbling blocks
#
# NOTE: New Arduino board presets (e.g. a future ESP32/ESP32-C3 target) should be
# added to CMakeLists.txt's ARDUINO_BOARD_PRESET options FIRST, then mirrored here
# (a new REQUIRED_CORE, and - for a third-party core like ESP32's - a
# `arduino-cli config add board_manager.additional_urls <url>` step) - keep this
# script's supported-board list in sync with CMakeLists.txt's, not ahead of it.
#
# Usage: ./install-prerequisites.sh
#
# Supported platforms:
# - Ubuntu/Debian (apt)
# - Fedora/RHEL/CentOS (dnf/yum)
# - macOS (Homebrew)

set -e

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

# Refresh arduino-cli's package index before installing any core - required on
# a first run (there is no index to install from yet).
echo "Updating arduino-cli package index..."
arduino-cli core update-index

# Install the board core for every preset CMakeLists.txt currently supports.
# arduino:avr covers "uno"/"mega"; arduino:renesas_uno covers "uno_r4"/
# "uno_r4_minima" - see CMakeLists.txt's ARDUINO_BOARD_PRESET options.
for core in "arduino:avr" "arduino:renesas_uno"; do
    if arduino-cli core list | grep -q "^${core} "; then
        echo -e "${GREEN}✓ $core core already installed${NC}"
    else
        echo "Installing $core core..."
        arduino-cli core install "$core"
    fi
done

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
    for core in "arduino:avr" "arduino:renesas_uno"; do
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
