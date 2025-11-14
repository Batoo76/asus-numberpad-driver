#!/bin/bash
# Test device detection without running the full driver

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "=========================================="
echo "ASUS NumberPad Driver - Device Detection Test"
echo "=========================================="
echo ""

if [ ! -f "bin/asus-numberpad-driver" ]; then
    echo "Error: Driver not built. Run: make"
    exit 1
fi

echo "1. Checking /proc/bus/input/devices..."
echo "-----------------------------------"
if [ ! -r "/proc/bus/input/devices" ]; then
    echo "Error: Cannot read /proc/bus/input/devices"
    exit 1
fi

echo "Searching for ASUS touchpad devices..."
echo ""

# Show all potential touchpads
echo "Potential touchpad devices:"
grep -B 2 -A 8 "ASUE\|ASUP\|ASUF\|ELAN" /proc/bus/input/devices | \
    grep -E "Name=|Handlers=|S: |H: " | \
    head -20

echo ""
echo "2. Testing device detection (dry run)..."
echo "-----------------------------------"
echo "This will show what devices would be detected:"
echo ""

# We can't easily test the C function directly, but we can show the input
cat /proc/bus/input/devices | grep -A 10 "ASUE\|ASUP\|ASUF\|ELAN" | head -30

echo ""
echo "3. Checking I2C devices..."
echo "-----------------------------------"
if ls /dev/i2c-* >/dev/null 2>&1; then
    echo "Available I2C devices:"
    for dev in /dev/i2c-*; do
        if [ -r "$dev" ]; then
            echo "  $dev (readable)"
        else
            echo "  $dev (not readable - may need permissions)"
        fi
    done
else
    echo "No I2C devices found"
fi

echo ""
echo "4. Testing with i2cdetect (if available)..."
echo "-----------------------------------"
if command -v i2cdetect >/dev/null 2>&1; then
    for i2c_bus in /sys/bus/i2c/devices/i2c-*/name; do
        if [ -f "$i2c_bus" ]; then
            bus_num=$(echo "$i2c_bus" | sed 's/.*i2c-\([0-9]*\).*/\1/')
            bus_name=$(cat "$i2c_bus" 2>/dev/null || echo "unknown")
            echo "I2C bus $bus_num: $bus_name"
            if [ -r "/dev/i2c-$bus_num" ]; then
                echo "  Scanning for devices..."
                sudo i2cdetect -y "$bus_num" 2>/dev/null | head -5 || echo "  (requires root or i2c group)"
            fi
        fi
    done
else
    echo "i2cdetect not available (install i2c-tools)"
fi

echo ""
echo "=========================================="
echo "Device detection test complete"
echo "=========================================="

