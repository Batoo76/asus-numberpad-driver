#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <syslog.h>
#include "numberpad.h"

/* Check if touch is in top-right icon area */
bool is_pressed_top_right_icon(int x, int y) {
    if (g_layout.top_right_icon_width <= 0 || g_layout.top_right_icon_height <= 0) {
        return false;
    }

    if (x >= g_touchpad.maxx - g_layout.top_right_icon_width &&
        y >= 0 && y <= g_layout.top_right_icon_height) {
        return true;
    }

    return false;
}

/* Check if touch is in top-left icon area */
bool is_pressed_top_left_icon(int x, int y) {
    if (g_layout.top_left_icon_width <= 0 || g_layout.top_left_icon_height <= 0) {
        return false;
    }

    if (x >= 0 && x <= g_layout.top_left_icon_width &&
        y >= 0 && y <= g_layout.top_left_icon_height) {
        return true;
    }

    return false;
}

/* Check if numlock has been held longer than activation time */
bool takes_numlock_longer_than_activation_time(void) {
    if (g_mt.numlock_touch_start_time == 0) {
        return false;
    }

    double press_duration = difftime(time(NULL), g_mt.numlock_touch_start_time);

    if (press_duration > g_config.activation_time) {
        syslog(LOG_INFO, "Numlock held for %.2f seconds (activation time: %.2f)",
               press_duration, g_config.activation_time);
        return true;
    }

    return false;
}

/* Check if top-left icon has been held longer than activation time */
bool takes_top_left_icon_longer_than_activation_time(void) {
    if (g_mt.top_left_icon_touch_start_time == 0) {
        return false;
    }

    double press_duration = difftime(time(NULL), g_mt.top_left_icon_touch_start_time);

    if (press_duration > g_config.top_left_icon_activation_time) {
        syslog(LOG_INFO, "Top-left icon held for %.2f seconds (activation time: %.2f)",
               press_duration, g_config.top_left_icon_activation_time);
        /* Reset timer for smooth brightness cycling */
        g_mt.top_left_icon_touch_start_time = time(NULL);
        return true;
    }

    return false;
}

/* Check if finger has slid from top-right icon */
bool is_slided_from_top_right_icon(int x, int y, int prev_x, int prev_y) {
    if (g_mt.top_right_icon_touch_start_time == 0) {
        return false;
    }

    if (prev_x == -1 || prev_y == -1 || x == -1 || y == -1) {
        return false;
    }

    /* Check for non-fluent movement (cancels gesture) */
    if (x - prev_x > 25) {
        syslog(LOG_DEBUG, "Slide from top-right cancelled: X movement too large (%d -> %d)", prev_x, x);
        g_mt.top_right_icon_touch_start_time = 0;
        g_mt.numlock_touch_start_time = 0;
        return false;
    }

    if (y - prev_y < -25) {
        syslog(LOG_DEBUG, "Slide from top-right cancelled: Y movement too large (%d -> %d)", prev_y, y);
        g_mt.top_right_icon_touch_start_time = 0;
        g_mt.numlock_touch_start_time = 0;
        return false;
    }

    /* Check if slide distance exceeds activation radius */
    int dx = g_touchpad.maxx - x;
    int dy = y;
    double distance_squared = dx * dx + dy * dy;
    double radius_squared = g_config.top_right_icon_slide_func_activation_radius *
                           g_config.top_right_icon_slide_func_activation_radius;

    if (distance_squared > radius_squared) {
        syslog(LOG_INFO, "Slide from top-right icon exceeded activation threshold (radius: %d)",
               g_config.top_right_icon_slide_func_activation_radius);
        g_mt.top_right_icon_touch_start_time = 0;
        g_mt.numlock_touch_start_time = 0;
        return true;
    }

    return false;
}

/* Check if finger has slid from top-left icon */
bool is_slided_from_top_left_icon(int x, int y, int prev_x, int prev_y) {
    if (g_mt.top_left_icon_touch_start_time == 0) {
        return false;
    }

    if (prev_x == -1 || prev_y == -1 || x == -1 || y == -1) {
        return false;
    }

    /* Check for non-fluent movement (cancels gesture) */
    if (x - prev_x < -25) {
        syslog(LOG_DEBUG, "Slide from top-left cancelled: X movement too large (%d -> %d)", prev_x, x);
        g_mt.top_left_icon_touch_start_time = 0;
        return false;
    }

    if (y - prev_y < -25) {
        syslog(LOG_DEBUG, "Slide from top-left cancelled: Y movement too large (%d -> %d)", prev_y, y);
        g_mt.top_left_icon_touch_start_time = 0;
        return false;
    }

    /* Check if slide distance exceeds activation radius */
    double distance_squared = x * x + y * y;
    double radius_squared = g_config.top_left_icon_slide_func_activation_radius *
                           g_config.top_left_icon_slide_func_activation_radius;

    if (distance_squared > radius_squared) {
        syslog(LOG_INFO, "Slide from top-left icon exceeded activation threshold (radius: %d)",
               g_config.top_left_icon_slide_func_activation_radius);
        g_mt.top_left_icon_touch_start_time = 0;
        return true;
    }

    return false;
}

/* Reset current multitouch slot */
void reset_current_mt_slot(void) {
    int slot = g_mt.current_slot;
    if (slot < 0 || slot >= MAX_MT_SLOTS) {
        return;
    }

    g_mt.x_values[slot] = -1;
    g_mt.y_values[slot] = -1;
    g_mt.x_init_values[slot] = -1;
    g_mt.y_init_values[slot] = -1;
    g_mt.x_previous_values[slot] = -1;
    g_mt.y_previous_values[slot] = -1;
    g_mt.numpad_key[slot] = NULL;
    g_mt.grab_status[slot] = false;

    /* Reset timers if this was the active slot */
    if (slot == g_mt.current_slot) {
        g_mt.numlock_touch_start_time = 0;
        g_mt.top_left_icon_touch_start_time = 0;
        g_mt.top_right_icon_touch_start_time = 0;
    }
}

/* Toggle numlock state */
void toggle_numlock(void) {
    pthread_mutex_lock(&g_touchpad.numlock_lock);

    if (g_touchpad.numlock) {
        /* Deactivate numpad */
        g_touchpad.numlock = false;
        syslog(LOG_INFO, "Numpad deactivated");

        /* Send numlock key release if configured */
        if (g_config.numpad_disables_sys_numlock) {
            send_numlock_key(0);
            send_numlock_key(1); /* Press and release */
            send_numlock_key(0);
        }
    } else {
        /* Activate numpad */
        g_touchpad.numlock = true;
        syslog(LOG_INFO, "Numpad activated");

        /* Set backlight if available */
        if (g_layout.backlight_levels_count > 0 && g_config.brightness > 0) {
            int brightness_index = 0;
            /* Find brightness index in backlight_levels */
            for (int i = 0; i < g_layout.backlight_levels_count; i++) {
                if (g_layout.backlight_levels[i] == (uint8_t)g_config.brightness) {
                    brightness_index = i;
                    break;
                }
            }
            if (brightness_index < g_layout.backlight_levels_count) {
                i2c_send_value(g_touchpad.device_id, g_touchpad.device_addr,
                              g_layout.backlight_levels[brightness_index]);
            }
        }
    }

    pthread_mutex_unlock(&g_touchpad.numlock_lock);
}

/* Increase brightness (cycle through levels) */
void increase_brightness(void) {
    if (g_layout.backlight_levels_count == 0) {
        return;
    }

    /* Find current brightness index */
    int current_index = 0;
    for (int i = 0; i < g_layout.backlight_levels_count; i++) {
        if (g_layout.backlight_levels[i] == (uint8_t)g_config.brightness) {
            current_index = i;
            break;
        }
    }

    /* Cycle to next brightness */
    current_index = (current_index + 1) % g_layout.backlight_levels_count;
    g_config.brightness = g_layout.backlight_levels[current_index];

    syslog(LOG_INFO, "Increased brightness to level %d (0x%02x)", current_index, g_config.brightness);

    /* Send to hardware */
    i2c_send_value(g_touchpad.device_id, g_touchpad.device_addr,
                   g_layout.backlight_levels[current_index]);
}

