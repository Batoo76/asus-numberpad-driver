#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include "numberpad.h"

/* Debug log level */
typedef enum {
    LOG_LEVEL_ERR = 0,
    LOG_LEVEL_INFO = 1,
    LOG_LEVEL_DEBUG = 2
} log_level_t;

static log_level_t g_log_level = LOG_LEVEL_INFO;
static bool g_log_to_stderr = false;

/* Initialize debug logging */
void debug_init(void) {
    const char *log_env = getenv("LOG");
    if (log_env) {
        if (strcasecmp(log_env, "DEBUG") == 0) {
            g_log_level = LOG_LEVEL_DEBUG;
        } else if (strcasecmp(log_env, "INFO") == 0) {
            g_log_level = LOG_LEVEL_INFO;
        } else if (strcasecmp(log_env, "ERR") == 0 || strcasecmp(log_env, "ERROR") == 0) {
            g_log_level = LOG_LEVEL_ERR;
        }
    }

    /* Also check for verbose flag */
    if (getenv("VERBOSE") != NULL) {
        g_log_level = LOG_LEVEL_DEBUG;
        g_log_to_stderr = true;
    }
}

/* Debug log function */
void debug_log(int priority, const char *format, ...) {
    va_list args;
    char buffer[512];
    time_t now;
    struct tm *tm_info;
    char time_str[64];

    /* Check if we should log this level */
    if (priority == LOG_DEBUG && g_log_level < LOG_LEVEL_DEBUG) {
        return;
    }
    if (priority == LOG_INFO && g_log_level < LOG_LEVEL_INFO) {
        return;
    }

    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    /* Log to syslog */
    syslog(priority, "%s", buffer);

    /* Also log to stderr if verbose mode */
    if (g_log_to_stderr) {
        time(&now);
        tm_info = localtime(&now);
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        const char *level_str = "INFO";
        if (priority == LOG_DEBUG) level_str = "DEBUG";
        else if (priority == LOG_ERR) level_str = "ERROR";

        fprintf(stderr, "[%s] [%s] %s\n", time_str, level_str, buffer);
        fflush(stderr);
    }
}

/* Print debug information about current state */
void debug_print_state(void) {
    debug_log(LOG_DEBUG, "=== Driver State ===");
    debug_log(LOG_DEBUG, "NumLock: %s", g_touchpad.numlock ? "ON" : "OFF");
    debug_log(LOG_DEBUG, "Idled: %s", g_touchpad.is_idled ? "YES" : "NO");
    debug_log(LOG_DEBUG, "Touchpad: %dx%d (numpad: %dx%d)",
              g_touchpad.maxx - g_touchpad.minx,
              g_touchpad.maxy - g_touchpad.miny,
              g_touchpad.maxx_numpad - g_touchpad.minx_numpad,
              g_touchpad.maxy_numpad - g_touchpad.miny_numpad);
    debug_log(LOG_DEBUG, "Current slot: %d", g_mt.current_slot);
    debug_log(LOG_DEBUG, "Brightness: 0x%02x", g_config.brightness);
    debug_log(LOG_DEBUG, "Activation time: %.2f s", g_config.activation_time);
}

/* Print event information */
void debug_print_event(const struct input_event *ev) {
    if (g_log_level < LOG_LEVEL_DEBUG) {
        return;
    }

    const char *type_str = "UNKNOWN";
    const char *code_str = "";

    if (ev->type == EV_ABS) {
        type_str = "EV_ABS";
        switch (ev->code) {
            case ABS_MT_POSITION_X: code_str = "ABS_MT_POSITION_X"; break;
            case ABS_MT_POSITION_Y: code_str = "ABS_MT_POSITION_Y"; break;
            case ABS_MT_SLOT: code_str = "ABS_MT_SLOT"; break;
            case ABS_MT_TRACKING_ID: code_str = "ABS_MT_TRACKING_ID"; break;
            default: code_str = "ABS_OTHER"; break;
        }
    } else if (ev->type == EV_KEY) {
        type_str = "EV_KEY";
        code_str = "KEY_*";
    } else if (ev->type == EV_MSC) {
        type_str = "EV_MSC";
        if (ev->code == MSC_TIMESTAMP) {
            code_str = "MSC_TIMESTAMP";
        }
    } else if (ev->type == EV_SYN) {
        type_str = "EV_SYN";
        code_str = "SYN_REPORT";
    }

    debug_log(LOG_DEBUG, "Event: %s %s value=%d", type_str, code_str, ev->value);
}

