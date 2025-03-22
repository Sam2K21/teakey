// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
#include <qp.h>
#include "RGLight20.qff.h"
#include "timer.h"


// Custom Keycodes
enum custom_keycodes {
    TIMER_KEY = SAFE_RANGE,
    LAYER_CYCLE,
};

// Display variables
static painter_device_t display;
static painter_font_handle_t my_font;
static bool timer_running = false;
static uint32_t timer_start = 0;
static uint32_t elapsed_time = 0;
static uint32_t start_time = 0;  // Track the starting time for resuming
static uint8_t current_layer = 0;


// Function to update OLED display with timer and layer indicator
void update_display(void) {
    qp_clear(display); // Clear the display

    // Convert elapsed time to hours, minutes, and seconds
    uint16_t hours = (elapsed_time / 3600000) % 100;  // Max 99 hours
    uint16_t minutes = (elapsed_time / 60000) % 60;
    uint16_t seconds = (elapsed_time / 1000) % 60;

    char timer_display[9];  // "00:00:00"
    snprintf(timer_display, sizeof(timer_display), "%02d:%02d:%02d", hours, minutes, seconds);
    qp_drawtext(display, 5, 5, my_font, timer_display);  // Adjusted positioning

    // Updated Timer Circle Position (Moved Right 4px)
    int timer_circle_x = 113;  // Shifted Right
    int timer_circle_y = 16;   // Unchanged Y-position
    qp_circle(display, timer_circle_x, timer_circle_y, 13, 0, 0, 255, false); // 27px diameter (13px radius)

    // Updated Play/Pause Logic (Switched Icons)
    if (timer_running) {
        // Pause Symbol (Two Vertical Bars) now appears when **running**
        qp_rect(display, timer_circle_x - 4, timer_circle_y - 5, timer_circle_x - 2, timer_circle_y + 5, 0, 0, 255, true);
        qp_rect(display, timer_circle_x + 2, timer_circle_y - 5, timer_circle_x + 4, timer_circle_y + 5, 0, 0, 255, true);
    } else {
        // Play Symbol (Triangle) now appears when **paused**
        qp_line(display, timer_circle_x - 3, timer_circle_y - 5, timer_circle_x - 3, timer_circle_y + 5, 0, 0, 255);
        qp_line(display, timer_circle_x - 3, timer_circle_y - 5, timer_circle_x + 5, timer_circle_y, 0, 0, 255);
        qp_line(display, timer_circle_x - 3, timer_circle_y + 5, timer_circle_x + 5, timer_circle_y, 0, 0, 255);
    }

    // Draw Layer Indicator
    char layer_display[10];
    snprintf(layer_display, sizeof(layer_display), "Layer %d", current_layer);
    qp_drawtext(display, 5, 40, my_font, layer_display);

    // Updated Layer Circle Position (Moved Right 4px, Up 1px)
    int layer_circle_x = 113;  // Shifted Right
    int layer_circle_y = 50;   // Shifted Up 1px
    qp_circle(display, layer_circle_x, layer_circle_y, 13, 0, 0, 255, false); // 27px diameter (13px radius)

    // Draw Layer Icon inside the Layer Circle (Simple Stack Symbol)
    qp_rect(display, layer_circle_x - 5, layer_circle_y - 5, layer_circle_x + 5, layer_circle_y - 3, 0, 0, 255, true);
    qp_rect(display, layer_circle_x - 3, layer_circle_y - 2, layer_circle_x + 3, layer_circle_y, 0, 0, 255, true);
    qp_rect(display, layer_circle_x - 1, layer_circle_y + 2, layer_circle_x + 1, layer_circle_y + 4, 0, 0, 255, true);

    qp_flush(display); // Flush updates to OLED
}



// Keycode Processing
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case TIMER_KEY:
    if (record->event.pressed) {
        timer_start = timer_read();  // Store press time
    }
    if (!record->event.pressed) {  // On release
        uint16_t hold_duration = timer_elapsed(timer_start); // Check how long button was held

        if (hold_duration > 500) {  // Long Press → Reset Timer
            elapsed_time = 0;  // Reset elapsed time
            timer_running = false;
        } else {  // Short Press → Toggle Play/Pause
            if (timer_running) { 
                // PAUSING: Store elapsed time
                elapsed_time = timer_read32() - start_time; 
                timer_running = false;
            } else { 
                // RESUMING: Ensure timer resumes from last paused time
                start_time = timer_read32() - elapsed_time;
                timer_running = true;
            }
        }
        update_display(); // Ensure OLED updates after change
    }
    return false;


        case LAYER_CYCLE:
            if (record->event.pressed) {
                current_layer = (current_layer + 1) % 5;
                layer_move(current_layer);
                update_display();
            }
            return false;
    }
    return true;
}

// Continuously update timer if running
void housekeeping_task_user(void) {
    if (timer_running) {
        // Only update elapsed_time while the timer is running
        elapsed_time = timer_read32() - start_time;
        update_display(); // Refresh both timer & layer indicator
    }
}


// OLED & Font Initialization
void keyboard_post_init_user(void) {
    i2cInit();  // Initialize I2C

    display = qp_sh1106_make_i2c_device(128, 64, 0x3C);
    qp_init(display, QP_ROTATION_180); // Flip OLED display to correct orientation

    // Load custom font
    my_font = qp_load_font_mem(font_RGLight20);
    if (my_font == NULL) {
        return;
    }

    update_display(); // Initial display setup
}

// Define keymap layers
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, TIMER_KEY, KC_MPLY, LAYER_CYCLE),
    [1] = LAYOUT(KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, TIMER_KEY, KC_MPLY, LAYER_CYCLE),
    [2] = LAYOUT(KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, TIMER_KEY, KC_MPLY, LAYER_CYCLE),
    [3] = LAYOUT(KC_1, KC_2, KC_3, KC_4, KC_5, KC_6, TIMER_KEY, KC_MPLY, LAYER_CYCLE)
};

// Encoder map (unchanged)
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [0] = { ENCODER_CCW_CW(KC_A, KC_B), ENCODER_CCW_CW(KC_W, KC_L) },
    [1] = { ENCODER_CCW_CW(KC_A, KC_B), ENCODER_CCW_CW(KC_W, KC_L) },
    [2] = { ENCODER_CCW_CW(KC_A, KC_B), ENCODER_CCW_CW(KC_W, KC_L) },
    [3] = { ENCODER_CCW_CW(KC_A, KC_B), ENCODER_CCW_CW(KC_W, KC_L) }
};
#endif