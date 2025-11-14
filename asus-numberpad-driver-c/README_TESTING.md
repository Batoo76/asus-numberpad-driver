# Testing and Debugging - Quick Start

## Setup Test Environment

```bash
cd asus-numberpad-driver-c
./test_setup.sh
```

This will:
- ✅ Check all dependencies
- ✅ Verify permissions
- ✅ Build the driver
- ✅ Create test directories

## Quick Tests

### 1. Test Device Detection
```bash
./test_device_detection.sh
```

### 2. Run Driver in Debug Mode
```bash
./debug_driver.sh
```

### 3. Monitor Events
```bash
./test_monitor_events.sh
```

## Debug Modes

### Verbose Output
```bash
VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config
```

### Debug Log Level
```bash
LOG=DEBUG ./bin/asus-numberpad-driver up5401ea test/config
```

### Watch System Logs
```bash
# Terminal 1: Run driver
./debug_driver.sh

# Terminal 2: Watch logs
journalctl -f | grep asus-numberpad-driver
```

## Documentation

- **Full Debugging Guide**: `docs/DEBUGGING.md`
- **Testing Guide**: `TESTING.md`

## Test Checklist

Before installation, verify:
- [ ] Driver compiles
- [ ] Devices are detected
- [ ] Driver starts without errors
- [ ] NumLock activation works
- [ ] Keys are sent correctly
- [ ] No crashes or errors

## Common Issues

**"Can't find touchpad"**
- Run: `./test_device_detection.sh`
- Check: `/proc/bus/input/devices`

**"Permission denied"**
- Add to groups: `sudo usermod -a -G input,i2c,uinput $USER`
- Log out and back in

**"Module not found"**
- Load: `sudo modprobe uinput i2c-dev`

For more details, see `docs/DEBUGGING.md`.

