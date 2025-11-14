# ASUS NumberPad Driver - C Implementation

This is a native C implementation of the ASUS NumberPad driver, converted from the Python version.

## Status

**⚠️ WORK IN PROGRESS**

This is an initial conversion project. The following components are implemented:

- ✅ Project structure and build system
- ✅ Configuration file parsing
- ✅ I2C communication (with fallback to i2ctransfer)
- ⏳ Input device handling (libevdev)
- ⏳ uinput virtual device creation
- ⏳ Touchpad event processing
- ⏳ X11/Wayland support
- ⏳ Layout loading
- ⏳ Main event loop

## Building

### Prerequisites

Install required development packages:

```bash
# Debian/Ubuntu
sudo apt-get install build-essential libevdev-dev libx11-dev \
    libwayland-dev libxkbcommon-dev libsystemd-dev

# Arch Linux
sudo pacman -S base-devel libevdev libx11 wayland libxkbcommon systemd-libs

# Fedora
sudo dnf install gcc libevdev-devel libX11-devel wayland-devel \
    libxkbcommon-devel systemd-devel
```

### Build

```bash
make
```

The binary will be created at `bin/asus-numberpad-driver`.

### Clean

```bash
make clean
```

## Usage

```bash
./bin/asus-numberpad-driver <layout_name> [config_dir]
```

Example:
```bash
./bin/asus-numberpad-driver up5401ea /usr/share/asus-numberpad-driver
```

## Project Structure

```
asus-numberpad-driver-c/
├── src/
│   ├── main.c              # Entry point
│   ├── config.c            # Configuration management
│   ├── i2c_control.c       # I2C backlight control
│   ├── input_handler.c     # libevdev/uinput (TODO)
│   ├── touchpad.c          # Touchpad event processing (TODO)
│   ├── numpad.c            # Numpad key mapping (TODO)
│   ├── x11_support.c       # X11 keyboard layout (TODO)
│   └── wayland_support.c   # Wayland keyboard layout (TODO)
├── include/
│   └── numberpad.h         # Main header
├── layouts/                # Layout definitions (TODO)
├── Makefile
└── README.md
```

## Conversion Progress

### Completed
- [x] Project structure
- [x] Build system (Makefile)
- [x] Configuration file parsing
- [x] I2C communication

### In Progress
- [ ] Input device handling (libevdev)
- [ ] uinput virtual device
- [ ] Touchpad event loop
- [ ] Layout loading system

### TODO
- [ ] X11 keyboard layout detection
- [ ] Wayland keyboard layout detection
- [ ] Multitouch handling
- [ ] Gesture recognition
- [ ] Systemd integration
- [ ] File watching (inotify)
- [ ] All configuration options
- [ ] Testing

## Differences from Python Version

1. **Memory Management**: Manual memory management in C vs automatic in Python
2. **Error Handling**: Explicit error checking vs exceptions
3. **Threading**: pthread API vs Python threading module
4. **Configuration**: Custom INI parser vs configparser
5. **Logging**: syslog vs Python logging

## Contributing

This is a conversion project. The goal is to maintain feature parity with the Python version while providing better performance and native Linux integration.

## License

Same as the original Python project (GPL v2).

