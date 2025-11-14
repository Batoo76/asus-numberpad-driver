#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <libevdev-1.0/libevdev/libevdev-uinput.h>
#include <linux/uinput.h>
#include "numberpad.h"

static struct libevdev *g_touchpad_dev = NULL;
static struct libevdev_uinput *g_uinput_dev = NULL;

/* Note: Keys are enabled when creating the uinput device.
 * This function is kept for API compatibility but doesn't need to do anything
 * since we enable all needed keys during device creation.
 */
int enable_uinput_key(unsigned int keycode) {
    (void)keycode; /* Unused for now */
    return 0;
}

/* Create uinput device */
int create_uinput_device(const char *touchpad_name) {
    struct libevdev *dev;
    int rc;

    dev = libevdev_new();
    if (!dev) {
        syslog(LOG_ERR, "Failed to create libevdev device");
        return -1;
    }

    /* Set device name */
    libevdev_set_name(dev, touchpad_name);

    /* Enable basic event types */
    libevdev_enable_event_type(dev, EV_KEY);
    libevdev_enable_event_type(dev, EV_MSC);
    libevdev_enable_event_type(dev, EV_SYN);

    /* Enable MSC_SCAN for key events */
    libevdev_enable_event_code(dev, EV_MSC, MSC_SCAN, NULL);

    /* Enable pointer buttons */
    libevdev_enable_event_code(dev, EV_KEY, BTN_LEFT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_RIGHT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, BTN_MIDDLE, NULL);

    /* Enable numpad keys */
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP0, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP1, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP2, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP3, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP4, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP5, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP6, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP7, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP8, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KP9, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPDOT, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPENTER, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPPLUS, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPMINUS, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPASTERISK, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPSLASH, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_KPEQUAL, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_NUMLOCK, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_BACKSPACE, NULL);
    libevdev_enable_event_code(dev, EV_KEY, KEY_CALC, NULL);

    /* Create uinput device */
    rc = libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED, &g_uinput_dev);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to create uinput device: %s", strerror(-rc));
        libevdev_free(dev);
        return -1;
    }

    syslog(LOG_INFO, "Created uinput device: %s", libevdev_get_name(dev));
    libevdev_free(dev);

    /* Sleep to allow udev to initialize the device */
    usleep(500000); /* 0.5 seconds */

    return 0;
}

/* Open touchpad device */
int open_touchpad_device(int event_num) {
    char path[64];
    int fd;
    int rc;

    snprintf(path, sizeof(path), "/dev/input/event%d", event_num);

    fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open touchpad device %s: %s", path, strerror(errno));
        return -1;
    }

    rc = libevdev_new_from_fd(fd, &g_touchpad_dev);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to create libevdev device from fd: %s", strerror(-rc));
        close(fd);
        return -1;
    }

    syslog(LOG_INFO, "Opened touchpad device: %s", libevdev_get_name(g_touchpad_dev));

    /* Get touchpad dimensions */
    if (libevdev_has_event_code(g_touchpad_dev, EV_ABS, ABS_X)) {
        const struct input_absinfo *absinfo = libevdev_get_abs_info(g_touchpad_dev, ABS_X);
        g_touchpad.minx = absinfo->minimum;
        g_touchpad.maxx = absinfo->maximum;
    }

    if (libevdev_has_event_code(g_touchpad_dev, EV_ABS, ABS_Y)) {
        const struct input_absinfo *absinfo = libevdev_get_abs_info(g_touchpad_dev, ABS_Y);
        g_touchpad.miny = absinfo->minimum;
        g_touchpad.maxy = absinfo->maximum;
    }

    syslog(LOG_INFO, "Touchpad dimensions: x %d-%d, y %d-%d",
           g_touchpad.minx, g_touchpad.maxx, g_touchpad.miny, g_touchpad.maxy);

    g_touchpad.touchpad_fd = fd;
    return 0;
}

/* Send key event via uinput */
int send_key_event(unsigned int keycode, int value) {
    if (!g_uinput_dev) {
        return -1;
    }

    int rc;

    /* Send MSC_SCAN before key event */
    rc = libevdev_uinput_write_event(g_uinput_dev, EV_MSC, MSC_SCAN, keycode);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to write MSC_SCAN event: %s", strerror(-rc));
        return -1;
    }

    /* Send key event */
    rc = libevdev_uinput_write_event(g_uinput_dev, EV_KEY, keycode, value);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to write key event: %s", strerror(-rc));
        return -1;
    }

    /* Send SYN_REPORT */
    rc = libevdev_uinput_write_event(g_uinput_dev, EV_SYN, SYN_REPORT, 0);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to write SYN_REPORT: %s", strerror(-rc));
        return -1;
    }

    return 0;
}

/* Send numlock key */
int send_numlock_key(int value) {
    /* TODO: Get actual numlock keycode from keyboard layout */
    return send_key_event(KEY_NUMLOCK, value);
}

/* Read next event from touchpad */
int read_touchpad_event(struct input_event *ev) {
    if (!g_touchpad_dev) {
        return -1;
    }

    int rc = libevdev_next_event(g_touchpad_dev, LIBEVDEV_READ_FLAG_NORMAL, ev);
    if (rc == LIBEVDEV_READ_STATUS_SUCCESS) {
        return 0;
    } else if (rc == -EAGAIN) {
        return 1; /* No event available */
    } else {
        syslog(LOG_ERR, "Error reading event: %s", strerror(-rc));
        return -1;
    }
}

/* Grab touchpad device */
int grab_touchpad(void) {
    if (!g_touchpad_dev) {
        return -1;
    }

    int rc = libevdev_grab(g_touchpad_dev, LIBEVDEV_GRAB);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to grab touchpad: %s", strerror(-rc));
        return -1;
    }

    syslog(LOG_INFO, "Grabbed touchpad device");
    return 0;
}

/* Ungrab touchpad device */
int ungrab_touchpad(void) {
    if (!g_touchpad_dev) {
        return -1;
    }

    int rc = libevdev_grab(g_touchpad_dev, LIBEVDEV_UNGRAB);
    if (rc < 0) {
        syslog(LOG_ERR, "Failed to ungrab touchpad: %s", strerror(-rc));
        return -1;
    }

    syslog(LOG_INFO, "Ungrabbed touchpad device");
    return 0;
}

/* Cleanup input devices */
void cleanup_input_devices(void) {
    if (g_uinput_dev) {
        libevdev_uinput_destroy(g_uinput_dev);
        g_uinput_dev = NULL;
    }

    if (g_touchpad_dev) {
        libevdev_free(g_touchpad_dev);
        g_touchpad_dev = NULL;
    }

    if (g_touchpad.touchpad_fd >= 0) {
        close(g_touchpad.touchpad_fd);
        g_touchpad.touchpad_fd = -1;
    }
}

