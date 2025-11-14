# Debugging Guide - ASUS NumberPad Driver

This guide explains how to test and debug the C driver before installation.

## Quick Start

1. **Setup test environment:**
   ```bash
   cd asus-numberpad-driver-c
   chmod +x test_setup.sh test_device_detection.sh debug_driver.sh
   ./test_setup.sh
   ```

2. **Test device detection:**
   ```bash
   ./test_device_detection.sh
   ```

3. **Run driver in debug mode:**
   ```bash
   ./debug_driver.sh
   ```

## Test Environment Setup

### Prerequisites Check

The `test_setup.sh` script checks:
- ✅ Build dependencies (gcc, make, pkg-config)
- ✅ Development libraries (libevdev, X11, Wayland, etc.)
- ✅ User groups (input, i2c, uinput)
- ✅ Kernel modules (uinput, i2c-dev)
- ✅ Device access permissions
- ✅ ASUS touchpad detection

### Fixing Common Issues

#### Missing User Groups
```bash
sudo usermod -a -G input,i2c,uinput $USER
# Log out and back in for changes to take effect
```

#### Missing Kernel Modules
```bash
sudo modprobe uinput
sudo modprobe i2c-dev
```

To make permanent:
```bash
echo "uinput" | sudo tee -a /etc/modules
echo "i2c-dev" | sudo tee -a /etc/modules
```

#### Permission Issues
If you can't access `/dev/uinput` or `/dev/i2c-*`:
- Add udev rules (see installation scripts from Python version)
- Or run with `sudo` for testing (not recommended for production)

## Debugging Modes

### 1. Verbose Logging

Enable verbose output to stderr:
```bash
VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config
```

Or set log level:
```bash
LOG=DEBUG ./bin/asus-numberpad-driver up5401ea test/config
LOG=INFO ./bin/asus-numberpad-driver up5401ea test/config
LOG=ERR ./bin/asus-numberpad-driver up5401ea test/config
```

### 2. System Log Monitoring

Watch system logs in real-time:
```bash
# Systemd journal
journalctl -f | grep asus-numberpad-driver

# Or syslog
tail -f /var/log/syslog | grep asus-numberpad-driver
```

### 3. Using debug_driver.sh

The debug script provides a convenient wrapper:
```bash
./debug_driver.sh                    # Default settings
./debug_driver.sh -l g533            # Different layout
./debug_driver.sh -c /path/to/config # Custom config
./debug_driver.sh -s                 # Run with sudo
```

## Testing Without Hardware

### Device Detection Test

Test if the driver can detect your touchpad:
```bash
./test_device_detection.sh
```

This shows:
- Available touchpad devices
- I2C bus information
- Device permissions

### Manual Device Inspection

Check `/proc/bus/input/devices`:
```bash
cat /proc/bus/input/devices | grep -A 10 "ASUE\|ASUP\|ASUF\|ELAN"
```

Check I2C devices:
```bash
ls -l /dev/i2c-*
i2cdetect -l  # List I2C buses
```

## Debugging Specific Issues

### Issue: Driver Fails to Start

1. **Check device detection:**
   ```bash
   ./test_device_detection.sh
   ```

2. **Check logs:**
   ```bash
   LOG=DEBUG ./bin/asus-numberpad-driver up5401ea test/config 2>&1 | tee debug.log
   ```

3. **Verify permissions:**
   ```bash
   ls -l /dev/uinput /dev/i2c-*
   groups  # Check if in input, i2c, uinput groups
   ```

### Issue: Touchpad Not Detected

1. **Verify touchpad exists:**
   ```bash
   cat /proc/bus/input/devices | grep -i touchpad
   ```

2. **Check device name patterns:**
   The driver looks for: `ASUE`, `ASUP`, `ASUF`, `ELAN` in device names

3. **Manual detection test:**
   ```bash
   # Find touchpad event number
   cat /proc/bus/input/devices | grep -B 5 -A 10 "Touchpad"
   ```

### Issue: I2C Communication Fails

1. **Check I2C device:**
   ```bash
   ls -l /dev/i2c-*
   sudo i2cdetect -y 1  # Replace 1 with your I2C bus number
   ```

2. **Test I2C manually:**
   ```bash
   # Find device ID from /proc/bus/input/devices (S: line)
   # Then test:
   sudo i2ctransfer -f -y 1 w13@0x15 0x05 0x00 0x3d 0x03 0x06 0x00 0x07 0x00 0x0d 0x14 0x03 0x41 0xad
   ```

3. **Check permissions:**
   ```bash
   groups | grep i2c
   ```

### Issue: uinput Device Not Created

1. **Check uinput module:**
   ```bash
   lsmod | grep uinput
   sudo modprobe uinput
   ```

2. **Check permissions:**
   ```bash
   ls -l /dev/uinput
   # Should be: crw-rw---- 1 root uinput
   ```

3. **Check groups:**
   ```bash
   groups | grep uinput
   ```

### Issue: Events Not Being Sent

1. **Enable debug logging:**
   ```bash
   LOG=DEBUG VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config
   ```

2. **Check if numpad is activated:**
   - Touch top-right icon and hold for 1 second
   - Or slide from top-right icon

3. **Monitor input events:**
   ```bash
   # In another terminal
   sudo evtest /dev/input/eventX  # Replace X with uinput device number
   ```

4. **Check if keys are being sent:**
   ```bash
   # Use xev (X11) or wev (Wayland) to see key events
   xev -event keyboard
   ```

## Debugging Tools

### 1. evtest

Monitor input events:
```bash
sudo apt-get install evtest
sudo evtest
# Select the uinput device (Asus Touchpad/Numpad)
```

### 2. libinput debug-events

Monitor libinput events:
```bash
sudo libinput debug-events
```

### 3. i2c-tools

Debug I2C communication:
```bash
sudo apt-get install i2c-tools
i2cdetect -l          # List I2C buses
i2cdetect -y 1        # Scan bus 1
i2cdump -y 1 0x15     # Dump device at 0x15
```

### 4. strace

Trace system calls:
```bash
strace -e trace=open,ioctl,write,read ./bin/asus-numberpad-driver up5401ea test/config
```

### 5. gdb

Debug with GDB:
```bash
gdb ./bin/asus-numberpad-driver
(gdb) set args up5401ea test/config
(gdb) break main
(gdb) run
(gdb) break numberpad_init
(gdb) continue
```

## Debug Output

### Log Levels

- **ERR/ERROR**: Only errors
- **INFO**: Informational messages (default)
- **DEBUG**: Detailed debug information including events

### Debug Information Printed

When `LOG=DEBUG` is set, the driver prints:
- All input events (type, code, value)
- Touch coordinates
- Icon detection
- Gesture recognition
- Key mapping
- State changes

### Example Debug Output

```
[2024-11-14 13:20:15] [DEBUG] Event: EV_ABS ABS_MT_SLOT value=0
[2024-11-14 13:20:15] [DEBUG] Event: EV_ABS ABS_MT_TRACKING_ID value=123
[2024-11-14 13:20:15] [DEBUG] Event: EV_ABS ABS_MT_POSITION_X value=2500
[2024-11-14 13:20:15] [DEBUG] Event: EV_ABS ABS_MT_POSITION_Y value=500
[2024-11-14 13:20:15] [DEBUG] Touched top-right icon
[2024-11-14 13:20:16] [INFO] Numlock held for 1.00 seconds (activation time: 1.00)
[2024-11-14 13:20:16] [INFO] Numpad activated
```

## Testing Checklist

Before installation, verify:

- [ ] Driver compiles without errors
- [ ] Device detection works (`test_device_detection.sh`)
- [ ] Driver starts without errors
- [ ] Touchpad is detected correctly
- [ ] I2C device is found
- [ ] uinput device is created
- [ ] Top-right icon touch is detected
- [ ] NumLock activation works (hold method)
- [ ] NumLock activation works (slide method)
- [ ] Numpad keys are sent when active
- [ ] Brightness control works (top-left icon)
- [ ] Config file is read correctly
- [ ] Logs are written correctly

## Common Problems and Solutions

### Problem: "Can't find touchpad"
**Solution**: Check `/proc/bus/input/devices` for your touchpad. The driver looks for specific name patterns.

### Problem: "Can't find device id"
**Solution**: The I2C device ID is extracted from the "S:" line in `/proc/bus/input/devices`. Check if the line contains `i2c-X`.

### Problem: "Failed to open I2C device"
**Solution**: 
- Check permissions: `ls -l /dev/i2c-*`
- Add user to i2c group: `sudo usermod -a -G i2c $USER`
- Or the driver will fall back to `i2ctransfer` command

### Problem: "Failed to create uinput device"
**Solution**:
- Load module: `sudo modprobe uinput`
- Check permissions: `ls -l /dev/uinput`
- Add user to uinput group: `sudo usermod -a -G uinput $USER`

### Problem: No events being received
**Solution**:
- Verify touchpad is working: `sudo evtest` (select touchpad device)
- Check if driver grabbed the device (may block other programs)
- Verify numpad is activated (touch top-right icon)

## Performance Debugging

### Check CPU Usage
```bash
top -p $(pgrep asus-numberpad-driver)
```

### Check Memory Usage
```bash
ps aux | grep asus-numberpad-driver
```

### Profile with perf
```bash
sudo perf record -g ./bin/asus-numberpad-driver up5401ea test/config
sudo perf report
```

## Reporting Issues

When reporting issues, include:

1. **System information:**
   ```bash
   uname -a
   lsb_release -a
   ```

2. **Device information:**
   ```bash
   ./test_device_detection.sh > device_info.txt
   ```

3. **Debug logs:**
   ```bash
   LOG=DEBUG VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config 2>&1 | tee debug.log
   ```

4. **Configuration:**
   - Layout name used
   - Config file contents
   - Any custom settings

5. **Steps to reproduce:**
   - What you did
   - What you expected
   - What actually happened

## Next Steps

After successful testing:
1. Review the logs for any warnings
2. Test all features (activation, gestures, brightness)
3. Compare behavior with Python version
4. Proceed with installation if everything works

