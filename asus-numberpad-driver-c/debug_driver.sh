#!/bin/bash
# Run driver in debug mode with verbose logging

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Default values
LAYOUT="up5401ea"
CONFIG_DIR="test/config"
LOG_LEVEL="DEBUG"
USE_SUDO=""

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -l|--layout)
            LAYOUT="$2"
            shift 2
            ;;
        -c|--config)
            CONFIG_DIR="$2"
            shift 2
            ;;
        -s|--sudo)
            USE_SUDO="sudo"
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -l, --layout LAYOUT    Layout name (default: up5401ea)"
            echo "  -c, --config DIR       Config directory (default: test/config)"
            echo "  -s, --sudo             Run with sudo"
            echo "  -h, --help             Show this help"
            echo ""
            echo "Environment variables:"
            echo "  LOG=DEBUG              Set log level (INFO, DEBUG, ERR)"
            echo "  G_MESSAGES_DEBUG=all   Enable GLib debug messages"
            echo ""
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ ! -f "bin/asus-numberpad-driver" ]; then
    echo "Error: Driver not built. Run: make"
    exit 1
fi

echo "=========================================="
echo "ASUS NumberPad Driver - Debug Mode"
echo "=========================================="
echo ""
echo "Layout: $LAYOUT"
echo "Config: $CONFIG_DIR"
echo "Log Level: $LOG_LEVEL"
echo ""

# Create config directory if it doesn't exist
mkdir -p "$CONFIG_DIR"

# Check if config file exists, create default if not
if [ ! -f "$CONFIG_DIR/numberpad_dev" ]; then
    echo "Creating default config file..."
    cat > "$CONFIG_DIR/numberpad_dev" << 'EOF'
[main]
enabled = 0
activation_time = 1.0
press_key_when_is_done_untouch = 1
numpad_disables_sys_numlock = 1
sys_numlock_enables_numpad = 1
EOF
fi

echo "Starting driver in debug mode..."
echo "Press Ctrl+C to stop"
echo ""
echo "Logs will appear in:"
echo "  - System log: journalctl -f | grep asus-numberpad-driver"
echo "  - Or: tail -f /var/log/syslog | grep asus-numberpad-driver"
echo ""

# Set environment variables for debugging
export LOG="$LOG_LEVEL"
export G_MESSAGES_DEBUG=all

# Run the driver
$USE_SUDO ./bin/asus-numberpad-driver "$LAYOUT" "$CONFIG_DIR"

