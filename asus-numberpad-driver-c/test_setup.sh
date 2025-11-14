#!/bin/bash
# Test environment setup script for ASUS NumberPad Driver

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "ASUS NumberPad Driver - Test Setup"
echo "=========================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if running as root
if [ "$EUID" -eq 0 ]; then 
   echo -e "${YELLOW}Warning: Running as root. Some checks may not work correctly.${NC}"
   echo ""
fi

# Function to check command availability
check_command() {
    if command -v "$1" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $1 is installed"
        return 0
    else
        echo -e "${RED}✗${NC} $1 is NOT installed"
        return 1
    fi
}

# Function to check file/directory permissions
check_permissions() {
    local path="$1"
    local required="$2"
    if [ -e "$path" ]; then
        if [ -$required "$path" ]; then
            echo -e "${GREEN}✓${NC} $path is accessible ($required)"
            return 0
        else
            echo -e "${RED}✗${NC} $path exists but not accessible ($required)"
            return 1
        fi
    else
        echo -e "${YELLOW}⚠${NC} $path does not exist"
        return 1
    fi
}

echo "1. Checking build dependencies..."
echo "-----------------------------------"
MISSING_DEPS=0

check_command gcc || MISSING_DEPS=1
check_command make || MISSING_DEPS=1
check_command pkg-config || MISSING_DEPS=1

# Check for development libraries
echo ""
echo "Checking for required libraries..."
pkg-config --exists libevdev 2>/dev/null && echo -e "${GREEN}✓${NC} libevdev development files" || { echo -e "${RED}✗${NC} libevdev development files missing"; MISSING_DEPS=1; }
pkg-config --exists x11 2>/dev/null && echo -e "${GREEN}✓${NC} X11 development files" || echo -e "${YELLOW}⚠${NC} X11 development files (optional for X11 support)"
pkg-config --exists wayland-client 2>/dev/null && echo -e "${GREEN}✓${NC} Wayland development files" || echo -e "${YELLOW}⚠${NC} Wayland development files (optional for Wayland support)"
pkg-config --exists xkbcommon 2>/dev/null && echo -e "${GREEN}✓${NC} xkbcommon development files" || echo -e "${YELLOW}⚠${NC} xkbcommon development files (optional)"
pkg-config --exists libsystemd 2>/dev/null && echo -e "${GREEN}✓${NC} systemd development files" || echo -e "${YELLOW}⚠${NC} systemd development files (optional)"

if [ $MISSING_DEPS -eq 1 ]; then
    echo ""
    echo -e "${RED}Missing required dependencies!${NC}"
    echo "Install with:"
    echo "  Debian/Ubuntu: sudo apt-get install build-essential libevdev-dev libx11-dev libwayland-dev libxkbcommon-dev libsystemd-dev"
    echo "  Arch: sudo pacman -S base-devel libevdev libx11 wayland libxkbcommon systemd-libs"
    exit 1
fi

echo ""
echo "2. Checking system permissions..."
echo "-----------------------------------"

# Check user groups
echo "Checking user groups..."
if groups | grep -q "\binput\b"; then
    echo -e "${GREEN}✓${NC} User is in 'input' group"
else
    echo -e "${YELLOW}⚠${NC} User is NOT in 'input' group (may need: sudo usermod -a -G input \$USER)"
fi

if groups | grep -q "\bi2c\b"; then
    echo -e "${GREEN}✓${NC} User is in 'i2c' group"
else
    echo -e "${YELLOW}⚠${NC} User is NOT in 'i2c' group (may need: sudo usermod -a -G i2c \$USER)"
fi

if groups | grep -q "\buinput\b"; then
    echo -e "${GREEN}✓${NC} User is in 'uinput' group"
else
    echo -e "${YELLOW}⚠${NC} User is NOT in 'uinput' group (may need: sudo usermod -a -G uinput \$USER)"
fi

echo ""
echo "3. Checking kernel modules..."
echo "-----------------------------------"
if lsmod | grep -q "^uinput"; then
    echo -e "${GREEN}✓${NC} uinput module is loaded"
else
    echo -e "${YELLOW}⚠${NC} uinput module not loaded (may need: sudo modprobe uinput)"
fi

if lsmod | grep -q "^i2c_dev"; then
    echo -e "${GREEN}✓${NC} i2c-dev module is loaded"
else
    echo -e "${YELLOW}⚠${NC} i2c-dev module not loaded (may need: sudo modprobe i2c-dev)"
fi

echo ""
echo "4. Checking device access..."
echo "-----------------------------------"
check_permissions "/dev/uinput" "r" || echo "  Note: May need udev rules or run with sudo"
check_permissions "/proc/bus/input/devices" "r" || echo "  Note: Should be readable by all users"

# Check for I2C devices
I2C_DEVICES=$(ls /dev/i2c-* 2>/dev/null | wc -l)
if [ "$I2C_DEVICES" -gt 0 ]; then
    echo -e "${GREEN}✓${NC} Found $I2C_DEVICES I2C device(s)"
    ls -l /dev/i2c-* 2>/dev/null | head -3
else
    echo -e "${YELLOW}⚠${NC} No I2C devices found"
fi

echo ""
echo "5. Checking for ASUS touchpad..."
echo "-----------------------------------"
if [ -r "/proc/bus/input/devices" ]; then
    if grep -q "ASUE\|ASUP\|ASUF\|ELAN" /proc/bus/input/devices 2>/dev/null; then
        echo -e "${GREEN}✓${NC} Found potential ASUS touchpad in /proc/bus/input/devices"
        echo "  Touchpad devices:"
        grep -A 5 "ASUE\|ASUP\|ASUF\|ELAN" /proc/bus/input/devices | grep -E "Name=|Handlers=" | head -4
    else
        echo -e "${YELLOW}⚠${NC} No ASUS touchpad detected in /proc/bus/input/devices"
        echo "  This is normal if you don't have an ASUS laptop with NumberPad"
    fi
else
    echo -e "${RED}✗${NC} Cannot read /proc/bus/input/devices"
fi

echo ""
echo "6. Building driver..."
echo "-----------------------------------"
if [ -f "Makefile" ]; then
    make clean >/dev/null 2>&1 || true
    if make 2>&1 | tee /tmp/asus_driver_build.log; then
        echo -e "${GREEN}✓${NC} Driver built successfully"
        if [ -f "bin/asus-numberpad-driver" ]; then
            echo -e "${GREEN}✓${NC} Binary created: bin/asus-numberpad-driver"
            ls -lh bin/asus-numberpad-driver
        fi
    else
        echo -e "${RED}✗${NC} Build failed. Check /tmp/asus_driver_build.log"
        exit 1
    fi
else
    echo -e "${RED}✗${NC} Makefile not found"
    exit 1
fi

echo ""
echo "7. Creating test directories..."
echo "-----------------------------------"
mkdir -p test/config
mkdir -p test/logs
echo -e "${GREEN}✓${NC} Test directories created"

# Create a test config file
if [ ! -f "test/config/numberpad_dev" ]; then
    cat > test/config/numberpad_dev << 'EOF'
[main]
enabled = 0
activation_time = 1.0
press_key_when_is_done_untouch = 1
numpad_disables_sys_numlock = 1
sys_numlock_enables_numpad = 1
EOF
    echo -e "${GREEN}✓${NC} Test config file created: test/config/numberpad_dev"
fi

echo ""
echo "=========================================="
echo -e "${GREEN}Test environment setup complete!${NC}"
echo "=========================================="
echo ""
echo "Next steps:"
echo "  1. Review the warnings above and fix any issues"
echo "  2. Run: ./test_device_detection.sh (to test device detection)"
echo "  3. Run: ./debug_driver.sh (to run driver in debug mode)"
echo "  4. Check: docs/DEBUGGING.md (for debugging guide)"
echo ""

