#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include "numberpad.h"

static bool g_running = true;

void signal_handler(int sig) {
    (void)sig;
    g_running = false;
    g_stop_threads = true;
}

void print_usage(const char *progname) {
    fprintf(stderr, "Usage: %s <layout_name> [config_dir]\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "  layout_name: Name of the layout file (without .py extension)\n");
    fprintf(stderr, "               e.g., up5401ea, ux433fa, g533, etc.\n");
    fprintf(stderr, "  config_dir:  Optional directory containing config file 'numberpad_dev'\n");
    fprintf(stderr, "               (default: current working directory)\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Example: %s up5401ea /usr/share/asus-numberpad-driver\n", progname);
}

int main(int argc, char *argv[]) {
    const char *layout_name = NULL;
    const char *config_dir = ".";

    /* Parse arguments */
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    layout_name = argv[1];
    if (argc >= 3) {
        config_dir = argv[2];
    }

    /* Setup signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGUSR1, signal_handler);

    /* Initialize debug logging first */
    debug_init();

    /* Open syslog */
    openlog("asus-numberpad-driver", LOG_PID | LOG_CONS, LOG_USER);
    debug_log(LOG_INFO, "Starting ASUS NumberPad driver (C version)");

    /* Initialize driver */
    if (numberpad_init(layout_name, config_dir) != 0) {
        debug_log(LOG_ERR, "Failed to initialize driver");
        closelog();
        return 1;
    }

    /* Print initial state in debug mode */
    debug_print_state();

    /* Run main loop */
    debug_log(LOG_INFO, "Driver initialized, entering main loop");
    if (numberpad_run() != 0) {
        debug_log(LOG_ERR, "Driver error in main loop");
    }

    /* Cleanup */
    numberpad_cleanup();
    debug_log(LOG_INFO, "Driver stopped");
    closelog();

    return 0;
}

