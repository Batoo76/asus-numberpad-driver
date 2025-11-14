#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <regex.h>
#include <errno.h>
#include <syslog.h>
#include <libevdev/libevdev.h>
#include <linux/input.h>
#include "numberpad.h"

#define MAX_LINE_LEN 512
#define TRY_TIMES 5
#define TRY_SLEEP_MS 100

/* Detect touchpad and keyboard devices from /proc/bus/input/devices */
int detect_devices(int *touchpad_event_num, char *touchpad_name, size_t name_len,
                   int *keyboard_event_num, int *device_id, int *device_addr) {
    FILE *fp;
    char line[MAX_LINE_LEN];
    int touchpad_detected = 0;
    int keyboard_detected = 0;
    int try_times = TRY_TIMES;
    regex_t regex;
    regmatch_t matches[2];

    /* Compile regex for extracting i2c device ID */
    if (regcomp(&regex, ".*i2c-([0-9]+)/.*", REG_EXTENDED) != 0) {
        syslog(LOG_ERR, "Failed to compile regex");
        return -1;
    }

    while (try_times > 0) {
        touchpad_detected = 0;
        keyboard_detected = 0;
        *touchpad_event_num = -1;
        *keyboard_event_num = -1;
        *device_id = -1;
        *device_addr = 0x15; /* Default address */

        fp = fopen("/proc/bus/input/devices", "r");
        if (!fp) {
            syslog(LOG_ERR, "Failed to open /proc/bus/input/devices");
            regfree(&regex);
            return -1;
        }

        while (fgets(line, sizeof(line), fp)) {
            /* Look for touchpad */
            if (touchpad_detected == 0 &&
                (strstr(line, "Name=\"ASUE") || strstr(line, "Name=\"ELAN") ||
                 strstr(line, "Name=\"ASUP") || strstr(line, "Name=\"ASUF")) &&
                strstr(line, "Touchpad") && !strstr(line, "9009")) {
                touchpad_detected = 1;
                syslog(LOG_INFO, "Detecting touchpad from string: \"%s\"", line);

                /* Extract touchpad name */
                char *name_start = strstr(line, "Name=\"");
                if (name_start) {
                    name_start += 6; /* Skip "Name=\"" */
                    char *name_end = strchr(name_start, '"');
                    if (name_end) {
                        size_t name_size = name_end - name_start;
                        if (name_size < name_len) {
                            strncpy(touchpad_name, name_start, name_size);
                            touchpad_name[name_size] = '\0';
                        }
                    }
                }

                /* Check for special device addresses */
                if (strstr(line, "ASUF1416") || strstr(line, "ASUF1205") ||
                    strstr(line, "ASUF1204")) {
                    *device_addr = 0x38;
                }
            }

            if (touchpad_detected == 1) {
                /* Extract I2C device ID from S: line */
                if (strstr(line, "S: ") == line) {
                    if (regexec(&regex, line, 2, matches, 0) == 0) {
                        char id_str[16];
                        size_t len = matches[1].rm_eo - matches[1].rm_so;
                        if (len < sizeof(id_str)) {
                            strncpy(id_str, line + matches[1].rm_so, len);
                            id_str[len] = '\0';
                            *device_id = atoi(id_str);
                            syslog(LOG_INFO, "Set touchpad device id %d from %s",
                                   *device_id, line);
                        }
                    }
                }

                /* Extract event number from H: line */
                if (strstr(line, "H: ") == line) {
                    char *event_str = strstr(line, "event");
                    if (event_str) {
                        *touchpad_event_num = atoi(event_str + 5);
                        touchpad_detected = 2;
                        syslog(LOG_INFO, "Set touchpad event %d from %s",
                               *touchpad_event_num, line);
                    }
                }
            }

            /* Look for keyboard */
            if (keyboard_detected == 0 &&
                (strstr(line, "Name=\"AT Translated Set 2 keyboard") ||
                 ((strstr(line, "Name=\"ASUE") || strstr(line, "Name=\"Asus") ||
                   strstr(line, "Name=\"ASUP") || strstr(line, "Name=\"ASUF")) &&
                  strstr(line, "Keyboard")))) {
                keyboard_detected = 1;
                syslog(LOG_INFO, "Detecting keyboard from string: \"%s\"", line);
            }

            /* Check if keyboard has numlock LED */
            if (keyboard_detected == 1 && strstr(line, "H: ") == line) {
                char *event_str = strstr(line, "event");
                if (event_str) {
                    int kb_event = atoi(event_str + 5);
                    char path[64];
                    snprintf(path, sizeof(path), "/dev/input/event%d", kb_event);

                    int fd = open(path, O_RDONLY);
                    if (fd >= 0) {
                        struct libevdev *dev;
                        if (libevdev_new_from_fd(fd, &dev) == 0) {
                            if (libevdev_has_event_type(dev, EV_LED) &&
                                libevdev_has_event_code(dev, EV_LED, LED_NUML)) {
                                *keyboard_event_num = kb_event;
                                keyboard_detected = 2;
                                syslog(LOG_INFO, "Set keyboard event %d from %s",
                                       kb_event, line);
                            }
                            libevdev_free(dev);
                        }
                        close(fd);
                    }

                    if (keyboard_detected != 2) {
                        keyboard_detected = 0;
                    }
                }
            }
        }

        fclose(fp);

        if (touchpad_detected == 2 && (keyboard_detected == 2 || keyboard_detected == 0)) {
            if (*device_id < 0) {
                syslog(LOG_ERR, "Can't find device id");
                regfree(&regex);
                return -1;
            }
            if (keyboard_detected != 2) {
                syslog(LOG_WARNING, "Keyboard with numlock not found (optional)");
            }
            break;
        }

        try_times--;
        if (try_times > 0) {
            usleep(TRY_SLEEP_MS * 1000);
        }
    }

    regfree(&regex);

    if (touchpad_detected != 2) {
        syslog(LOG_ERR, "Can't find touchpad (code: %d)", touchpad_detected);
        return -1;
    }

    return 0;
}

