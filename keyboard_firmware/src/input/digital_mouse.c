#include "digital_mouse.h"
#include "fn_keys.h"
#include <string.h>

void digital_mouse_init(digital_mouse_t *mouse, uint32_t update_interval_ms) {
    memset(mouse, 0, sizeof(digital_mouse_t));
    mouse->update_interval_ms = update_interval_ms;
}

void digital_mouse_update_button(digital_mouse_t *mouse, uint8_t fn_key_index, bool pressed) {
    // Map FN keys to mouse movement
    // FN12: Left movement
    // FN11: Right movement
    // FN10: Up movement
    // FN9: Down movement
    // FN8: Handled separately as click action (generates FIFO event)
    
    switch (fn_key_index) {
        case FN_KEY_FN12:  // Left
            mouse->left_pressed = pressed;
            break;
            
        case FN_KEY_FN11:  // Right
            mouse->right_pressed = pressed;
            break;
            
        case FN_KEY_FN10:  // Up
            mouse->up_pressed = pressed;
            break;
            
        case FN_KEY_FN9:   // Down
            mouse->down_pressed = pressed;
            break;
            
        default:
            // Not a mouse control key
            break;
    }
}

void digital_mouse_tick(digital_mouse_t *mouse, uint32_t now_ms) {
    // Check if it's time to update position
    if ((now_ms - mouse->last_update_ms) < mouse->update_interval_ms) {
        return;
    }
    
    mouse->last_update_ms = now_ms;
    
    // Update X position based on left/right buttons
    if (mouse->left_pressed && !mouse->right_pressed) {
        mouse->x_position -= MOUSE_SPEED_NORMAL;
    } else if (mouse->right_pressed && !mouse->left_pressed) {
        mouse->x_position += MOUSE_SPEED_NORMAL;
    }
    
    // Update Y position based on up/down buttons
    // Note: Y is typically inverted (down = positive)
    if (mouse->up_pressed && !mouse->down_pressed) {
        mouse->y_position -= MOUSE_SPEED_NORMAL;
    } else if (mouse->down_pressed && !mouse->up_pressed) {
        mouse->y_position += MOUSE_SPEED_NORMAL;
    }
    
    // Clamp to signed 8-bit range to prevent overflow
    if (mouse->x_position > 127) {
        mouse->x_position = 127;
    } else if (mouse->x_position < -128) {
        mouse->x_position = -128;
    }
    
    if (mouse->y_position > 127) {
        mouse->y_position = 127;
    } else if (mouse->y_position < -128) {
        mouse->y_position = -128;
    }
}

int8_t digital_mouse_get_and_clear_x(digital_mouse_t *mouse) {
    int8_t x = (int8_t)mouse->x_position;
    mouse->x_position = 0;
    return x;
}

int8_t digital_mouse_get_and_clear_y(digital_mouse_t *mouse) {
    int8_t y = (int8_t)mouse->y_position;
    mouse->y_position = 0;
    return y;
}

void digital_mouse_reset(digital_mouse_t *mouse) {
    mouse->x_position = 0;
    mouse->y_position = 0;
}
