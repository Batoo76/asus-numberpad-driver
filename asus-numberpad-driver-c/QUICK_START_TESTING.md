# Quick Start - Testing the Driver

## 1. Setup (One Time)

```bash
cd asus-numberpad-driver-1/asus-numberpad-driver-c
chmod +x *.sh
./test_setup.sh
```

This checks dependencies, permissions, and builds the driver.

## 2. Test Device Detection

```bash
./test_device_detection.sh
```

Verifies your hardware is detected.

## 3. Run Driver in Debug Mode

```bash
./debug_driver.sh
```

Runs the driver with verbose output. Press Ctrl+C to stop.

## 4. Monitor Events (Optional)

In another terminal:
```bash
# Watch system logs
journalctl -f | grep asus-numberpad-driver

# Or monitor input events
sudo evtest
```

## Debug Options

### Verbose Output
```bash
VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config
```

### Debug Log Level
```bash
LOG=DEBUG ./bin/asus-numberpad-driver up5401ea test/config
```

### Both
```bash
VERBOSE=1 LOG=DEBUG ./bin/asus-numberpad-driver up5401ea test/config
```

## Testing Features

1. **NumLock Activation**
   - Touch top-right corner, hold 1 second
   - Or slide from top-right corner

2. **Numpad Keys**
   - Activate numpad first
   - Touch numpad area
   - Keys should be sent

3. **Brightness Control**
   - Activate numpad
   - Touch top-left corner, hold 1 second
   - Brightness cycles

## Troubleshooting

- **"Can't find touchpad"**: Run `./test_device_detection.sh`
- **"Permission denied"**: Add to groups: `sudo usermod -a -G input,i2c,uinput $USER`
- **"Module not found"**: `sudo modprobe uinput i2c-dev`

See `docs/DEBUGGING.md` for detailed help.

