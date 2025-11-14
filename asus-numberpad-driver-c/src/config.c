#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include "numberpad.h"

/* Default configuration values */
static void config_set_defaults(config_t *config) {
    config->enabled = 0;
    config->numpad_disables_sys_numlock = 1;
    config->disable_due_inactivity_time = 120;
    config->touchpad_disables_numpad = 1;
    config->key_repetitions = 0;
    config->multitouch = 0;
    config->one_touch_key_rotation = 0;
    config->activation_time = 1.0;
    config->sys_numlock_enables_numpad = 1;
    config->top_left_icon_activation_time = 1.0;
    config->top_left_icon_slide_func_activation_radius = 1200;
    config->top_left_icon_slide_func_activates_numpad = 1;
    config->top_left_icon_brightness_func_max_min_only = 1;
    config->top_right_icon_slide_func_activation_radius = 1200;
    config->enabled_touchpad_pointer = 3;
    config->press_key_when_is_done_untouch = 1;
    config->default_backlight_level = 0x01;
    config->top_left_icon_brightness_func_disabled = 0;
    config->brightness = 0x41;
    config->distance_to_move_only_pointer = 250;
    config->idled = 0;
    config->idle_brightness = 30;
    config->idle_time = 30;
    config->idle_enabled = 0;
    config->top_left_icon_slide_func_disabled = 0;
}

/* Simple INI parser - reads key=value pairs from [main] section */
int config_load(const char *config_file, config_t *config) {
    FILE *fp;
    char line[256];
    char key[128], value[128];
    bool in_main_section = false;

    /* Set defaults first */
    config_set_defaults(config);

    fp = fopen(config_file, "r");
    if (!fp) {
        syslog(LOG_WARNING, "Config file %s not found, using defaults", config_file);
        return 0; /* Not an error, just use defaults */
    }

    while (fgets(line, sizeof(line), fp)) {
        /* Remove whitespace */
        char *p = line;
        while (isspace(*p)) p++;
        if (*p == '\0' || *p == '#' || *p == ';') continue;

        /* Check for section */
        if (*p == '[') {
            in_main_section = (strncmp(p, "[main]", 6) == 0);
            continue;
        }

        if (!in_main_section) continue;

        /* Parse key=value */
        if (sscanf(p, "%127[^=]=%127s", key, value) == 2) {
            /* Trim whitespace from key */
            char *end = key + strlen(key) - 1;
            while (end > key && isspace(*end)) *end-- = '\0';

            /* Parse value */
            if (strcmp(key, "enabled") == 0) {
                config->enabled = atoi(value);
            } else if (strcmp(key, "numpad_disables_sys_numlock") == 0) {
                config->numpad_disables_sys_numlock = atoi(value);
            } else if (strcmp(key, "disable_due_inactivity_time") == 0) {
                config->disable_due_inactivity_time = atoi(value);
            } else if (strcmp(key, "touchpad_disables_numpad") == 0) {
                config->touchpad_disables_numpad = atoi(value);
            } else if (strcmp(key, "key_repetitions") == 0) {
                config->key_repetitions = atoi(value);
            } else if (strcmp(key, "multitouch") == 0) {
                config->multitouch = atoi(value);
            } else if (strcmp(key, "one_touch_key_rotation") == 0) {
                config->one_touch_key_rotation = atoi(value);
            } else if (strcmp(key, "activation_time") == 0) {
                config->activation_time = atof(value);
            } else if (strcmp(key, "sys_numlock_enables_numpad") == 0) {
                config->sys_numlock_enables_numpad = atoi(value);
            } else if (strcmp(key, "top_left_icon_activation_time") == 0) {
                config->top_left_icon_activation_time = atof(value);
            } else if (strcmp(key, "top_left_icon_slide_func_activation_radius") == 0) {
                config->top_left_icon_slide_func_activation_radius = atoi(value);
            } else if (strcmp(key, "top_left_icon_slide_func_activates_numpad") == 0) {
                config->top_left_icon_slide_func_activates_numpad = atoi(value);
            } else if (strcmp(key, "top_left_icon_brightness_func_max_min_only") == 0) {
                config->top_left_icon_brightness_func_max_min_only = atoi(value);
            } else if (strcmp(key, "top_right_icon_slide_func_activation_radius") == 0) {
                config->top_right_icon_slide_func_activation_radius = atoi(value);
            } else if (strcmp(key, "enabled_touchpad_pointer") == 0) {
                config->enabled_touchpad_pointer = atoi(value);
            } else if (strcmp(key, "press_key_when_is_done_untouch") == 0) {
                config->press_key_when_is_done_untouch = atoi(value);
            } else if (strcmp(key, "default_backlight_level") == 0) {
                config->default_backlight_level = (int)strtol(value, NULL, 16);
            } else if (strcmp(key, "top_left_icon_brightness_func_disabled") == 0) {
                config->top_left_icon_brightness_func_disabled = atoi(value);
            } else if (strcmp(key, "brightness") == 0) {
                config->brightness = (int)strtol(value, NULL, 16);
            } else if (strcmp(key, "distance_to_move_only_pointer") == 0) {
                config->distance_to_move_only_pointer = atoi(value);
            } else if (strcmp(key, "idled") == 0) {
                config->idled = atoi(value);
            } else if (strcmp(key, "idle_brightness") == 0) {
                config->idle_brightness = atoi(value);
            } else if (strcmp(key, "idle_time") == 0) {
                config->idle_time = atoi(value);
            } else if (strcmp(key, "idle_enabled") == 0) {
                config->idle_enabled = atoi(value);
            } else if (strcmp(key, "top_left_icon_slide_func_disabled") == 0) {
                config->top_left_icon_slide_func_disabled = atoi(value);
            }
        }
    }

    fclose(fp);
    syslog(LOG_INFO, "Loaded configuration from %s", config_file);
    return 0;
}

int config_save(const char *config_file, const config_t *config) {
    FILE *fp;

    fp = fopen(config_file, "w");
    if (!fp) {
        syslog(LOG_ERR, "Failed to open config file %s for writing", config_file);
        return -1;
    }

    fprintf(fp, "[main]\n");
    fprintf(fp, "enabled = %d\n", config->enabled);
    fprintf(fp, "numpad_disables_sys_numlock = %d\n", config->numpad_disables_sys_numlock);
    fprintf(fp, "disable_due_inactivity_time = %d\n", config->disable_due_inactivity_time);
    fprintf(fp, "touchpad_disables_numpad = %d\n", config->touchpad_disables_numpad);
    fprintf(fp, "key_repetitions = %d\n", config->key_repetitions);
    fprintf(fp, "multitouch = %d\n", config->multitouch);
    fprintf(fp, "one_touch_key_rotation = %d\n", config->one_touch_key_rotation);
    fprintf(fp, "activation_time = %.1f\n", config->activation_time);
    fprintf(fp, "sys_numlock_enables_numpad = %d\n", config->sys_numlock_enables_numpad);
    fprintf(fp, "top_left_icon_activation_time = %.1f\n", config->top_left_icon_activation_time);
    fprintf(fp, "top_left_icon_slide_func_activation_radius = %d\n", config->top_left_icon_slide_func_activation_radius);
    fprintf(fp, "top_left_icon_slide_func_activates_numpad = %d\n", config->top_left_icon_slide_func_activates_numpad);
    fprintf(fp, "top_left_icon_brightness_func_max_min_only = %d\n", config->top_left_icon_brightness_func_max_min_only);
    fprintf(fp, "top_right_icon_slide_func_activation_radius = %d\n", config->top_right_icon_slide_func_activation_radius);
    fprintf(fp, "enabled_touchpad_pointer = %d\n", config->enabled_touchpad_pointer);
    fprintf(fp, "press_key_when_is_done_untouch = %d\n", config->press_key_when_is_done_untouch);
    fprintf(fp, "default_backlight_level = 0x%02x\n", config->default_backlight_level);
    fprintf(fp, "top_left_icon_brightness_func_disabled = %d\n", config->top_left_icon_brightness_func_disabled);
    fprintf(fp, "brightness = 0x%02x\n", config->brightness);
    fprintf(fp, "distance_to_move_only_pointer = %d\n", config->distance_to_move_only_pointer);
    fprintf(fp, "idled = %d\n", config->idled);
    fprintf(fp, "idle_brightness = %d\n", config->idle_brightness);
    fprintf(fp, "idle_time = %d\n", config->idle_time);
    fprintf(fp, "idle_enabled = %d\n", config->idle_enabled);
    fprintf(fp, "top_left_icon_slide_func_disabled = %d\n", config->top_left_icon_slide_func_disabled);

    fclose(fp);
    syslog(LOG_INFO, "Saved configuration to %s", config_file);
    return 0;
}

