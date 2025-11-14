# Testing Guide - ASUS NumberPad Driver

Quick reference for testing the driver before installation.

## Quick Test

```bash
# 1. Setup environment
./test_setup.sh

# 2. Test device detection
./test_device_detection.sh

# 3. Run driver in debug mode
./debug_driver.sh
```

## Test Scripts

### test_setup.sh
Comprehensive environment check:
- Verifies all dependencies
- Checks permissions and groups
- Builds the driver
- Creates test directories

### test_device_detection.sh
Tests device detection without running the full driver:
- Shows available touchpads
- Lists I2C devices
- Displays device information

### debug_driver.sh
Runs the driver with debug output:
- Verbose logging to stderr
- Configurable log levels
- Easy configuration

### test_monitor_events.sh
Helps monitor input events:
- Lists available devices
- Shows how to use evtest
- Provides monitoring commands

## Manual Testing Steps

### 1. Build and Verify
```bash
cd asus-numberpad-driver-c
make clean && make
./bin/asus-numberpad-driver  # Should show usage
```

### 2. Test Device Detection
```bash
./test_device_detection.sh
# Verify touchpad is detected
# Verify I2C device is found
```

### 3. Run with Debug Output
```bash
LOG=DEBUG VERBOSE=1 ./bin/asus-numberpad-driver up5401ea test/config
```

### 4. Test NumLock Activation
- Touch top-right corner of touchpad
- Hold for 1 second
- Should see "Numpad activated" in logs
- Touch numpad keys - should send key events

### 5. Test Slide Gesture
- Touch top-right corner
- Slide finger toward center
- Should activate immediately

### 6. Test Brightness Control
- Activate numpad first
- Touch top-left corner
- Hold for 1 second
- Brightness should cycle

### 7. Monitor Events
In another terminal:
```bash
# Watch system logs
journalctl -f | grep asus-numberpad-driver

# Or monitor input events
sudo evtest
# Select the uinput device
```

## Expected Behavior

### On Startup
- Driver detects touchpad device
- Driver detects I2C device
- Driver creates uinput device
- Driver loads configuration
- Driver enters event loop

### During Operation
- Touch events are logged (DEBUG mode)
- Icon touches are detected
- Activation works after hold time
- Keys are sent when numpad is active

### On Shutdown
- Clean shutdown on Ctrl+C
- Resources are freed
- Logs show "Driver stopped"

## Troubleshooting

See `docs/DEBUGGING.md` for detailed troubleshooting guide.

## Success Criteria

Driver is ready for installation when:
- ✅ Compiles without errors
- ✅ Detects devices correctly
- ✅ Starts without errors
- ✅ NumLock activation works
- ✅ Keys are sent correctly
- ✅ No crashes or memory leaks
- ✅ Clean shutdown

