# Python to C Conversion Mapping

This document maps Python code patterns to their C equivalents for reference during conversion.

## Libraries and APIs

| Python | C Equivalent | Notes |
|--------|--------------|-------|
| `libevdev.Device()` | `libevdev_new_from_fd()` | Create device from file descriptor |
| `libevdev.InputEvent()` | `struct input_event` | Input event structure |
| `device.create_uinput_device()` | `libevdev_uinput_create_from_device()` | Create uinput device |
| `udev.send_events()` | `libevdev_uinput_write_event()` | Send events to uinput |
| `periphery.I2C()` | `open("/dev/i2c-N", O_RDWR)` + `ioctl()` | I2C communication |
| `Xlib.display.Display()` | `XOpenDisplay()` | X11 display connection |
| `pywayland.client.Display()` | `wl_display_connect()` | Wayland display connection |
| `xkbcommon.xkb` | `xkb_*` functions | Keyboard layout handling |
| `pyinotify` | `inotify_*` functions | File system watching |
| `configparser` | Custom parser or `inih` | INI file parsing |
| `threading.Thread()` | `pthread_create()` | Thread creation |
| `threading.Lock()` | `pthread_mutex_t` | Mutex for locking |
| `logging` | `syslog()` | System logging |
| `numpy` | Standard math functions | Mathematical operations |

## Data Structures

### Python Global Variables → C Global State

```python
# Python
numlock = False
enabled_evdev_keys = []
abs_mt_slot_x_values = [-1] * 5
```

```c
// C
bool g_touchpad.numlock = false;
int g_enabled_evdev_keys[MAX_KEYS];
int g_mt.x_values[MAX_MT_SLOTS];
```

### Python Lists → C Arrays

```python
# Python
backlight_levels = ["0x41", "0x42", "0x43"]
keys = [["7", "8", "9"], ["4", "5", "6"]]
```

```c
// C
uint8_t backlight_levels[] = {0x41, 0x42, 0x43};
int keys[ROWS][COLS] = {
    {KEY_KP7, KEY_KP8, KEY_KP9},
    {KEY_KP4, KEY_KP5, KEY_KP6}
};
```

## Common Patterns

### File I/O

```python
# Python
with open('/dev/input/event12', 'rb') as f:
    data = f.read()
```

```c
// C
int fd = open("/dev/input/event12", O_RDONLY);
if (fd < 0) {
    // error handling
}
read(fd, &event, sizeof(event));
close(fd);
```

### Error Handling

```python
# Python
try:
    result = some_function()
except Exception as e:
    log.error("Error: %s", e)
    return None
```

```c
// C
int result = some_function();
if (result < 0) {
    syslog(LOG_ERR, "Error: %s", strerror(errno));
    return -1;
}
```

### Threading

```python
# Python
import threading
lock = threading.Lock()

def worker():
    with lock:
        # critical section
        pass

t = threading.Thread(target=worker)
t.start()
```

```c
// C
#include <pthread.h>
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void* worker(void* arg) {
    pthread_mutex_lock(&lock);
    // critical section
    pthread_mutex_unlock(&lock);
    return NULL;
}

pthread_t thread;
pthread_create(&thread, NULL, worker, NULL);
```

### Configuration Parsing

```python
# Python
config = configparser.ConfigParser()
config.read('config.ini')
value = config.getint('main', 'enabled', fallback=0)
```

```c
// C
// Custom parser or use inih library
config_t config;
config_load("config.ini", &config);
int value = config.enabled;  // defaults set in config_set_defaults()
```

### I2C Communication

```python
# Python
from periphery import I2C
with I2C("/dev/i2c-1") as i2c:
    msg = I2C.Message([0x05, 0x00, ...])
    i2c.transfer(0x15, [msg])
```

```c
// C
#include <linux/i2c-dev.h>
int fd = open("/dev/i2c-1", O_RDWR);
ioctl(fd, I2C_SLAVE, 0x15);
uint8_t data[] = {0x05, 0x00, ...};
write(fd, data, sizeof(data));
close(fd);
```

### Event Reading

```python
# Python
from libevdev import Device
d_t = Device(fd)
for e in d_t.events():
    if e.matches(EV_ABS.ABS_MT_POSITION_X):
        x = e.value
```

```c
// C
#include <libevdev/libevdev.h>
struct libevdev *dev;
libevdev_new_from_fd(fd, &dev);

struct input_event ev;
while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0) {
    if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_X) {
        int x = ev.value;
    }
}
```

### String Operations

```python
# Python
path = f"/dev/i2c-{device_id}"
if "enabled" in value:
    pass
```

```c
// C
char path[32];
snprintf(path, sizeof(path), "/dev/i2c-%d", device_id);
if (strstr(value, "enabled") != NULL) {
    // found
}
```

## Key Conversion Points

1. **Memory Management**: Python is automatic, C requires manual `malloc()`/`free()`
2. **Error Handling**: Python uses exceptions, C uses return codes
3. **Type Safety**: Python is dynamic, C is static (need explicit types)
4. **String Handling**: Python strings are objects, C uses `char*` arrays
5. **Thread Safety**: Both support mutexes, but C requires explicit initialization
6. **Resource Management**: Python uses context managers, C uses explicit open/close

## Function Mapping Examples

### Python → C Function Signatures

```python
# Python
def send_value_to_touchpad_via_i2c(value):
    # implementation
    return True
```

```c
// C
int i2c_send_value(int device_id, int device_addr, uint8_t value) {
    // implementation
    return 0;  // 0 = success, -1 = error
}
```

```python
# Python
def config_get(key, key_default):
    return config.get('main', key, fallback=key_default)
```

```c
// C
int config_get_int(const char *key, int default_value) {
    // lookup in config structure
    return value;
}
```

## Notes

- C requires explicit memory management - be careful with allocations
- C uses 0 for success, negative for errors (common Linux convention)
- C strings are null-terminated `char*` arrays, not objects
- C arrays are fixed-size or require dynamic allocation
- C doesn't have exceptions - use return codes and `errno`
- C requires explicit type conversions (casts)
- C function parameters are pass-by-value (use pointers for modification)

