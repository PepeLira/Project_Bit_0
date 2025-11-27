#ifndef DIGITAL_MOUSE_H
#define DIGITAL_MOUSE_H

#include <stdbool.h>
#include <stdint.h>

// Mouse button indices (based on FN8-FN12)
#define MOUSE_BTN_LEFT_CLICK    0  // FN8 - left click
#define MOUSE_BTN_RIGHT_CLICK   1  // FN8 - right click (alternate)
#define MOUSE_BTN_MIDDLE_CLICK  2  // FN8 - middle click (alternate)
#define MOUSE_BTN_SCROLL_LEFT   3  // FN12 - scroll left
#define MOUSE_BTN_SCROLL_RIGHT  4  // FN11 - scroll right

// Mouse movement speed (pixels per tick when button held)
#define MOUSE_SPEED_NORMAL 2
#define MOUSE_SPEED_FAST 5

// Digital mouse state
typedef struct {
    int16_t x_position;        // Current X position/delta
    int16_t y_position;        // Current Y position/delta
    
    bool left_pressed;         // FN12 - move left
    bool right_pressed;        // FN11 - move right
    bool up_pressed;           // FN10 - move up
    bool down_pressed;         // FN9 - move down
    
    uint32_t last_update_ms;   // Last time position was updated
    uint32_t update_interval_ms; // How often to update position (e.g., 20ms)
} digital_mouse_t;

/**
 * Initialize the digital mouse.
 * 
 * @param mouse Pointer to digital mouse state
 * @param update_interval_ms Update interval for position accumulation
 */
void digital_mouse_init(digital_mouse_t *mouse, uint32_t update_interval_ms);

/**
 * Update mouse button state from FN key events.
 * 
 * @param mouse Pointer to digital mouse state
 * @param fn_key_index FN key index (FN_KEY_FN9 through FN_KEY_FN12 for movement)
 * @param pressed true if key is pressed, false if released
 */
void digital_mouse_update_button(digital_mouse_t *mouse, uint8_t fn_key_index, bool pressed);

/**
 * Update mouse position based on current button states.
 * Should be called regularly from main tick.
 * 
 * @param mouse Pointer to digital mouse state
 * @param now_ms Current time in milliseconds
 */
void digital_mouse_tick(digital_mouse_t *mouse, uint32_t now_ms);

/**
 * Get current X position/delta and reset it.
 * Used for I2C reporting.
 * 
 * @param mouse Pointer to digital mouse state
 * @return X position/delta (signed 8-bit)
 */
int8_t digital_mouse_get_and_clear_x(digital_mouse_t *mouse);

/**
 * Get current Y position/delta and reset it.
 * Used for I2C reporting.
 * 
 * @param mouse Pointer to digital mouse state
 * @return Y position/delta (signed 8-bit)
 */
int8_t digital_mouse_get_and_clear_y(digital_mouse_t *mouse);

/**
 * Reset mouse position to zero.
 * 
 * @param mouse Pointer to digital mouse state
 */
void digital_mouse_reset(digital_mouse_t *mouse);

#endif  // DIGITAL_MOUSE_H
