#ifndef NUMBERPAD_H
#define NUMBERPAD_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <linux/input.h>

/* Configuration structure */
typedef struct {
    int enabled;
    int numpad_disables_sys_numlock;
    int disable_due_inactivity_time;
    int touchpad_disables_numpad;
    int key_repetitions;
    int multitouch;
    int one_touch_key_rotation;
    double activation_time;
    int sys_numlock_enables_numpad;
    double top_left_icon_activation_time;
    int top_left_icon_slide_func_activation_radius;
    int top_left_icon_slide_func_activates_numpad;
    int top_left_icon_brightness_func_max_min_only;
    int top_right_icon_slide_func_activation_radius;
    int enabled_touchpad_pointer;
    int press_key_when_is_done_untouch;
    int default_backlight_level;
    int top_left_icon_brightness_func_disabled;
    int brightness;
    int distance_to_move_only_pointer;
    int idled;
    int idle_brightness;
    int idle_time;
    int idle_enabled;
    int top_left_icon_slide_func_disabled;
} config_t;

/* Layout structure */
typedef struct {
    int top_left_icon_width;
    int top_left_icon_height;
    int top_right_icon_width;
    int top_right_icon_height;
    int top_offset;
    int right_offset;
    int left_offset;
    int bottom_offset;
    int backlight_levels_count;
    uint8_t *backlight_levels;
    int keys_rows;
    int keys_cols;
    int **keys;  /* 2D array of key codes */
    int keys_ignore_offset_count;
    int *keys_ignore_offset;
} layout_t;

/* Touchpad state */
typedef struct {
    int device_id;
    int device_addr;
    int touchpad_fd;
    int keyboard_fd;
    int minx, maxx, miny, maxy;
    int minx_numpad, maxx_numpad, miny_numpad, maxy_numpad;
    double col_width, row_height;
    bool numlock;
    bool is_idled;
    time_t last_event_time;
    pthread_mutex_t numlock_lock;
    pthread_mutex_t idle_lock;
    pthread_mutex_t config_lock;
} touchpad_state_t;

/* Multitouch slot tracking */
#define MAX_MT_SLOTS 5

typedef struct {
    int x_values[MAX_MT_SLOTS];
    int y_values[MAX_MT_SLOTS];
    int x_init_values[MAX_MT_SLOTS];
    int y_init_values[MAX_MT_SLOTS];
    int x_previous_values[MAX_MT_SLOTS];
    int y_previous_values[MAX_MT_SLOTS];
    int *numpad_key[MAX_MT_SLOTS];
    int current_slot;
    time_t numlock_touch_start_time;
    time_t top_left_icon_touch_start_time;
    time_t top_right_icon_touch_start_time;
    bool grab_status[MAX_MT_SLOTS];
} mt_state_t;

/* Global state */
extern config_t g_config;
extern layout_t g_layout;
extern touchpad_state_t g_touchpad;
extern mt_state_t g_mt;
extern bool g_stop_threads;

/* Function declarations */
int numberpad_init(const char *layout_name, const char *config_dir);
void numberpad_cleanup(void);
int numberpad_run(void);

/* Device detection */
int detect_devices(int *touchpad_event_num, char *touchpad_name, size_t name_len,
                   int *keyboard_event_num, int *device_id, int *device_addr);

/* Input handling */
int open_touchpad_device(int event_num);
int create_uinput_device(const char *touchpad_name);
int send_key_event(unsigned int keycode, int value);
int send_numlock_key(int value);
int read_touchpad_event(struct input_event *ev);
int grab_touchpad(void);
int ungrab_touchpad(void);
void cleanup_input_devices(void);

/* Layout loading */
int load_layout(const char *layout_name, layout_t *layout);
void free_layout(layout_t *layout);

/* Configuration */
int config_load(const char *config_file, config_t *config);
int config_save(const char *config_file, const config_t *config);

/* I2C */
int i2c_send_value(int device_id, int device_addr, uint8_t value);

/* Touchpad gestures and icon detection */
bool is_pressed_top_right_icon(int x, int y);
bool is_pressed_top_left_icon(int x, int y);
bool takes_numlock_longer_than_activation_time(void);
bool takes_top_left_icon_longer_than_activation_time(void);
bool is_slided_from_top_right_icon(int x, int y, int prev_x, int prev_y);
bool is_slided_from_top_left_icon(int x, int y, int prev_x, int prev_y);
void reset_current_mt_slot(void);
void toggle_numlock(void);
void increase_brightness(void);

/* Debug functions */
void debug_init(void);
void debug_log(int priority, const char *format, ...);
void debug_print_state(void);
void debug_print_event(const struct input_event *ev);

#endif /* NUMBERPAD_H */

