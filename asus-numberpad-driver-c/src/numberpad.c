#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <pthread.h>
#include <time.h>
#include <math.h>
#include <linux/input.h>
#include "numberpad.h"

/* Global state */
config_t g_config;
layout_t g_layout;
touchpad_state_t g_touchpad;
mt_state_t g_mt;
bool g_stop_threads = false;

static char g_config_file[512] = "";

/* Initialize driver */
int numberpad_init(const char *layout_name, const char *config_dir) {
    /* Initialize mutexes */
    if (pthread_mutex_init(&g_touchpad.numlock_lock, NULL) != 0) {
        syslog(LOG_ERR, "Failed to initialize numlock mutex");
        return -1;
    }
    if (pthread_mutex_init(&g_touchpad.idle_lock, NULL) != 0) {
        syslog(LOG_ERR, "Failed to initialize idle mutex");
        pthread_mutex_destroy(&g_touchpad.numlock_lock);
        return -1;
    }
    if (pthread_mutex_init(&g_touchpad.config_lock, NULL) != 0) {
        syslog(LOG_ERR, "Failed to initialize config mutex");
        pthread_mutex_destroy(&g_touchpad.numlock_lock);
        pthread_mutex_destroy(&g_touchpad.idle_lock);
        return -1;
    }

    /* Build config file path */
    snprintf(g_config_file, sizeof(g_config_file), "%s/numberpad_dev", config_dir);

    /* Load configuration */
    if (config_load(g_config_file, &g_config) != 0) {
        syslog(LOG_ERR, "Failed to load configuration");
        numberpad_cleanup();
        return -1;
    }

    /* Save config if it didn't exist (to create defaults) */
    if (access(g_config_file, F_OK) != 0) {
        config_save(g_config_file, &g_config);
    }

    /* Detect devices */
    char touchpad_name[256] = "";
    int touchpad_event_num, keyboard_event_num;
    int device_id, device_addr;

    if (detect_devices(&touchpad_event_num, touchpad_name, sizeof(touchpad_name),
                       &keyboard_event_num, &device_id, &device_addr) != 0) {
        syslog(LOG_ERR, "Failed to detect devices");
        numberpad_cleanup();
        return -1;
    }

    g_touchpad.device_id = device_id;
    g_touchpad.device_addr = device_addr;

    /* Load layout */
    if (load_layout(layout_name, &g_layout) != 0) {
        syslog(LOG_ERR, "Failed to load layout: %s", layout_name);
        numberpad_cleanup();
        return -1;
    }

    /* Calculate numpad dimensions */
    g_touchpad.minx_numpad = g_touchpad.minx + g_layout.left_offset;
    g_touchpad.maxx_numpad = g_touchpad.maxx - g_layout.right_offset;
    g_touchpad.miny_numpad = g_touchpad.miny + g_layout.top_offset;
    g_touchpad.maxy_numpad = g_touchpad.maxy - g_layout.bottom_offset;

    double col_width = (double)(g_touchpad.maxx_numpad - g_touchpad.minx_numpad) / g_layout.keys_cols;
    double row_height = (double)(g_touchpad.maxy_numpad - g_touchpad.miny_numpad) / g_layout.keys_rows;
    g_touchpad.col_width = col_width;
    g_touchpad.row_height = row_height;

    syslog(LOG_INFO, "Numpad dimensions: x %d-%d, y %d-%d",
           g_touchpad.minx_numpad, g_touchpad.maxx_numpad,
           g_touchpad.miny_numpad, g_touchpad.maxy_numpad);

    /* Initialize touchpad state */
    g_touchpad.touchpad_fd = -1;
    g_touchpad.keyboard_fd = keyboard_event_num >= 0 ? keyboard_event_num : -1;
    g_touchpad.last_event_time = time(NULL);

    /* Open touchpad device */
    if (open_touchpad_device(touchpad_event_num) != 0) {
        syslog(LOG_ERR, "Failed to open touchpad device");
        free_layout(&g_layout);
        numberpad_cleanup();
        return -1;
    }

    /* Create uinput device */
    char uinput_name[256];
    snprintf(uinput_name, sizeof(uinput_name), "%s NumberPad", touchpad_name);
    if (create_uinput_device(uinput_name) != 0) {
        syslog(LOG_ERR, "Failed to create uinput device");
        cleanup_input_devices();
        free_layout(&g_layout);
        numberpad_cleanup();
        return -1;
    }

    /* Initialize multitouch state */
    memset(&g_mt, 0, sizeof(g_mt));
    for (int i = 0; i < MAX_MT_SLOTS; i++) {
        g_mt.x_values[i] = -1;
        g_mt.y_values[i] = -1;
        g_mt.x_init_values[i] = -1;
        g_mt.y_init_values[i] = -1;
        g_mt.x_previous_values[i] = -1;
        g_mt.y_previous_values[i] = -1;
        g_mt.numpad_key[i] = NULL;
    }

    g_touchpad.numlock = false;
    g_touchpad.is_idled = false;
    g_stop_threads = false;

    syslog(LOG_INFO, "Driver initialized successfully");
    return 0;
}

/* Cleanup driver */
void numberpad_cleanup(void) {
    syslog(LOG_INFO, "Cleaning up driver");

    cleanup_input_devices();

    if (g_touchpad.keyboard_fd >= 0) {
        close(g_touchpad.keyboard_fd);
        g_touchpad.keyboard_fd = -1;
    }

    free_layout(&g_layout);

    /* Destroy mutexes */
    pthread_mutex_destroy(&g_touchpad.numlock_lock);
    pthread_mutex_destroy(&g_touchpad.idle_lock);
    pthread_mutex_destroy(&g_touchpad.config_lock);

    syslog(LOG_INFO, "Cleanup complete");
}

/* Get touched key from coordinates */
static int get_touched_key(int x, int y) {
    if (x < g_touchpad.minx_numpad || x > g_touchpad.maxx_numpad ||
        y < g_touchpad.miny_numpad || y > g_touchpad.maxy_numpad) {
        return -1; /* Outside numpad area */
    }

    int col = (int)((x - g_touchpad.minx_numpad) / g_touchpad.col_width);
    int row = (int)((y - g_touchpad.miny_numpad) / g_touchpad.row_height);

    if (row >= 0 && row < g_layout.keys_rows && col >= 0 && col < g_layout.keys_cols) {
        return g_layout.keys[row][col];
    }

    return -1;
}

/* Main event loop */
int numberpad_run(void) {
    struct input_event ev;
    int current_key = -1;
    bool key_pressed = false;

    syslog(LOG_INFO, "Entering main event loop");

    while (!g_stop_threads) {
        int rc = read_touchpad_event(&ev);
        if (rc < 0) {
            /* Error reading event */
            break;
        } else if (rc > 0) {
            /* No event available, sleep a bit */
            usleep(10000); /* 10ms */
            continue;
        }

        g_touchpad.last_event_time = time(NULL);
        int slot = g_mt.current_slot;
        int x = g_mt.x_values[slot];
        int y = g_mt.y_values[slot];
        int prev_x = g_mt.x_previous_values[slot];
        int prev_y = g_mt.y_previous_values[slot];

        /* Print event in debug mode */
        debug_print_event(&ev);

        /* Process ABS_MT_SLOT */
        if (ev.type == EV_ABS && ev.code == ABS_MT_SLOT) {
            if (ev.value >= 0 && ev.value < MAX_MT_SLOTS) {
                g_mt.current_slot = ev.value;
                slot = ev.value;
            }
            continue;
        }

        /* Process ABS_MT_TRACKING_ID */
        if (ev.type == EV_ABS && ev.code == ABS_MT_TRACKING_ID) {
            if (ev.value >= 0) {
                /* Finger down - reset initial coordinates to capture first real positions */
                g_mt.x_init_values[slot] = -1;
                g_mt.y_init_values[slot] = -1;
            } else {
                /* Finger lifted */
                if (g_config.press_key_when_is_done_untouch && key_pressed && current_key >= 0) {
                    /* Send key on release */
                    send_key_event(current_key, 1);
                    send_key_event(current_key, 0);
                } else if (key_pressed && current_key >= 0) {
                    send_key_event(current_key, 0);
                }
                key_pressed = false;
                current_key = -1;
                reset_current_mt_slot();
            }
            continue;
        }

        /* Process MSC_TIMESTAMP - check for activation */
        if (ev.type == EV_MSC && ev.code == MSC_TIMESTAMP) {
            /* Check top-right icon (numlock) activation */
            if (is_pressed_top_right_icon(x, y) && takes_numlock_longer_than_activation_time()) {
                toggle_numlock();
                g_mt.numlock_touch_start_time = 0;
                g_mt.top_right_icon_touch_start_time = 0;
                continue;
            }

            /* Check top-left icon (brightness) activation */
            if (g_touchpad.numlock && is_pressed_top_left_icon(x, y) &&
                takes_top_left_icon_longer_than_activation_time() &&
                !g_config.top_left_icon_brightness_func_disabled &&
                g_layout.backlight_levels_count > 0) {
                increase_brightness();
                continue;
            }
        }

        /* Process ABS_MT_POSITION_X */
        if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_X) {
            /* Save previous value */
            g_mt.x_previous_values[slot] = g_mt.x_values[slot];
            g_mt.x_values[slot] = ev.value;
            x = ev.value;

            if (g_mt.x_init_values[slot] == -1) {
                g_mt.x_init_values[slot] = ev.value;
            }

            /* Check for slide gestures */
            if (is_slided_from_top_right_icon(x, y, prev_x, prev_y)) {
                toggle_numlock();
                continue;
            }

            if (is_slided_from_top_left_icon(x, y, prev_x, prev_y)) {
                /* TODO: Handle top-left slide (calculator function) */
                syslog(LOG_DEBUG, "Top-left icon slide gesture detected");
                continue;
            }

            /* Check if touching top-right icon */
            if (is_pressed_top_right_icon(x, y)) {
                if (g_mt.top_right_icon_touch_start_time == 0) {
                    g_mt.top_right_icon_touch_start_time = time(NULL);
                    g_mt.numlock_touch_start_time = time(NULL);
                    syslog(LOG_DEBUG, "Touched top-right icon");
                }
            }

            /* Check if touching top-left icon */
            if (is_pressed_top_left_icon(x, y)) {
                if (g_mt.top_left_icon_touch_start_time == 0) {
                    g_mt.top_left_icon_touch_start_time = time(NULL);
                    syslog(LOG_DEBUG, "Touched top-left icon");
                }
            }

            /* Process numpad keys if numlock is enabled */
            if (g_touchpad.numlock) {
                int key = get_touched_key(ev.value, y);
                if (key >= 0 && key != current_key) {
                    /* New key touched */
                    if (key_pressed && current_key >= 0) {
                        send_key_event(current_key, 0);
                    }
                    current_key = key;
                    key_pressed = true;
                    if (g_config.press_key_when_is_done_untouch) {
                        /* Don't send key yet, wait for release */
                    } else {
                        send_key_event(current_key, 1);
                    }
                }
            }
        }
        /* Process ABS_MT_POSITION_Y */
        else if (ev.type == EV_ABS && ev.code == ABS_MT_POSITION_Y) {
            /* Save previous value */
            g_mt.y_previous_values[slot] = g_mt.y_values[slot];
            g_mt.y_values[slot] = ev.value;
            y = ev.value;

            if (g_mt.y_init_values[slot] == -1) {
                g_mt.y_init_values[slot] = ev.value;
            }

            /* Check for slide gestures */
            if (is_slided_from_top_right_icon(x, y, prev_x, prev_y)) {
                toggle_numlock();
                continue;
            }

            if (is_slided_from_top_left_icon(x, y, prev_x, prev_y)) {
                /* TODO: Handle top-left slide (calculator function) */
                syslog(LOG_DEBUG, "Top-left icon slide gesture detected");
                continue;
            }

            /* Process numpad keys if numlock is enabled */
            if (g_touchpad.numlock) {
                int key = get_touched_key(x, ev.value);
                if (key >= 0 && key != current_key) {
                    if (key_pressed && current_key >= 0) {
                        send_key_event(current_key, 0);
                    }
                    current_key = key;
                    key_pressed = true;
                    if (g_config.press_key_when_is_done_untouch) {
                        /* Don't send key yet, wait for release */
                    } else {
                        send_key_event(current_key, 1);
                    }
                }
            }
        }
    }

    syslog(LOG_INFO, "Exiting main event loop");
    return 0;
}

