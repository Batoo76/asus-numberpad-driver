#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <linux/input.h>
#include "numberpad.h"

/* Map key name string to keycode */
static int key_name_to_code(const char *name) {
    if (strcmp(name, "0") == 0) return KEY_KP0;
    if (strcmp(name, "1") == 0) return KEY_KP1;
    if (strcmp(name, "2") == 0) return KEY_KP2;
    if (strcmp(name, "3") == 0) return KEY_KP3;
    if (strcmp(name, "4") == 0) return KEY_KP4;
    if (strcmp(name, "5") == 0) return KEY_KP5;
    if (strcmp(name, "6") == 0) return KEY_KP6;
    if (strcmp(name, "7") == 0) return KEY_KP7;
    if (strcmp(name, "8") == 0) return KEY_KP8;
    if (strcmp(name, "9") == 0) return KEY_KP9;
    if (strcmp(name, "period") == 0 || strcmp(name, ".") == 0) return KEY_KPDOT;
    if (strcmp(name, "Return") == 0 || strcmp(name, "Enter") == 0) return KEY_KPENTER;
    if (strcmp(name, "plus") == 0 || strcmp(name, "+") == 0) return KEY_KPPLUS;
    if (strcmp(name, "minus") == 0 || strcmp(name, "-") == 0) return KEY_KPMINUS;
    if (strcmp(name, "asterisk") == 0 || strcmp(name, "*") == 0) return KEY_KPASTERISK;
    if (strcmp(name, "slash") == 0 || strcmp(name, "/") == 0) return KEY_KPSLASH;
    if (strcmp(name, "equal") == 0 || strcmp(name, "=") == 0) return KEY_KPEQUAL;
    if (strcmp(name, "BackSpace") == 0 || strcmp(name, "Backspace") == 0) return KEY_BACKSPACE;
    if (strcmp(name, "percent") == 0 || strcmp(name, "%") == 0) return -1; /* Unicode */
    if (strcmp(name, "#") == 0) return -1; /* Unicode */
    return -1; /* Unknown or unicode character */
}

/* Load layout from hardcoded data (for now) */
/* TODO: Load from file or convert Python layouts */
int load_layout(const char *layout_name, layout_t *layout) {
    /* For now, hardcode up5401ea layout as example */
    if (strcmp(layout_name, "up5401ea") == 0) {
        layout->top_left_icon_width = 250;
        layout->top_left_icon_height = 250;
        layout->top_right_icon_width = 250;
        layout->top_right_icon_height = 250;
        layout->top_offset = 200;
        layout->right_offset = 200;
        layout->left_offset = 200;
        layout->bottom_offset = 80;

        /* Backlight levels */
        layout->backlight_levels_count = 8;
        layout->backlight_levels = malloc(layout->backlight_levels_count * sizeof(uint8_t));
        if (!layout->backlight_levels) {
            syslog(LOG_ERR, "Failed to allocate memory for backlight levels");
            return -1;
        }
        layout->backlight_levels[0] = 0x41;
        layout->backlight_levels[1] = 0x42;
        layout->backlight_levels[2] = 0x43;
        layout->backlight_levels[3] = 0x44;
        layout->backlight_levels[4] = 0x45;
        layout->backlight_levels[5] = 0x46;
        layout->backlight_levels[6] = 0x47;
        layout->backlight_levels[7] = 0x48;

        /* Keys layout: 4 rows x 5 columns */
        layout->keys_rows = 4;
        layout->keys_cols = 5;
        layout->keys = malloc(layout->keys_rows * sizeof(int*));
        if (!layout->keys) {
            free(layout->backlight_levels);
            syslog(LOG_ERR, "Failed to allocate memory for keys");
            return -1;
        }

        for (int i = 0; i < layout->keys_rows; i++) {
            layout->keys[i] = malloc(layout->keys_cols * sizeof(int));
            if (!layout->keys[i]) {
                /* Free already allocated rows */
                for (int j = 0; j < i; j++) {
                    free(layout->keys[j]);
                }
                free(layout->keys);
                free(layout->backlight_levels);
                syslog(LOG_ERR, "Failed to allocate memory for keys row");
                return -1;
            }
        }

        /* Row 0: 7, 8, 9, /, BackSpace */
        layout->keys[0][0] = KEY_KP7;
        layout->keys[0][1] = KEY_KP8;
        layout->keys[0][2] = KEY_KP9;
        layout->keys[0][3] = KEY_KPSLASH;
        layout->keys[0][4] = KEY_BACKSPACE;

        /* Row 1: 4, 5, 6, *, BackSpace */
        layout->keys[1][0] = KEY_KP4;
        layout->keys[1][1] = KEY_KP5;
        layout->keys[1][2] = KEY_KP6;
        layout->keys[1][3] = KEY_KPASTERISK;
        layout->keys[1][4] = KEY_BACKSPACE;

        /* Row 2: 1, 2, 3, -, % (unicode) */
        layout->keys[2][0] = KEY_KP1;
        layout->keys[2][1] = KEY_KP2;
        layout->keys[2][2] = KEY_KP3;
        layout->keys[2][3] = KEY_KPMINUS;
        layout->keys[2][4] = -1; /* Unicode % */

        /* Row 3: 0, ., Enter, +, = */
        layout->keys[3][0] = KEY_KP0;
        layout->keys[3][1] = KEY_KPDOT;
        layout->keys[3][2] = KEY_KPENTER;
        layout->keys[3][3] = KEY_KPPLUS;
        layout->keys[3][4] = KEY_KPEQUAL;

        layout->keys_ignore_offset_count = 0;
        layout->keys_ignore_offset = NULL;

        syslog(LOG_INFO, "Loaded layout: %s", layout_name);
        return 0;
    }

    syslog(LOG_ERR, "Unknown layout: %s", layout_name);
    return -1;
}

/* Free layout resources */
void free_layout(layout_t *layout) {
    if (layout->backlight_levels) {
        free(layout->backlight_levels);
        layout->backlight_levels = NULL;
    }

    if (layout->keys) {
        for (int i = 0; i < layout->keys_rows; i++) {
            if (layout->keys[i]) {
                free(layout->keys[i]);
            }
        }
        free(layout->keys);
        layout->keys = NULL;
    }

    if (layout->keys_ignore_offset) {
        free(layout->keys_ignore_offset);
        layout->keys_ignore_offset = NULL;
    }
}

