#include "../hardware/button.h"
#include "../config/config.h"
#include "../input/digital_mouse.h"
#include "../input/fn_keys.h"
#include "../hardware/i2c_slave.h"
#include "../input/key_fifo.h"
#include "led_controller.h"
#include "../input/matrix_scanner.h"
#include "../input/modifier_manager.h"
#include "pico/stdlib.h"
#include "../hardware/power_latch.h"
#include "../input/switch_tracker.h"
#include "../core/tick.h"

static void process_switch_event(switch_event_t event, uint32_t now_ms) {
    switch (event) {
        case SWITCH_EVENT_FIRST_PRESS:
            break;
        case SWITCH_EVENT_LONG_PRESS:
            power_latch_open();
            break;
        case SWITCH_EVENT_SHORT_PRESS:
            led_controller_pulse_short_press(now_ms);
            break;
        case SWITCH_EVENT_NONE:
        default:
            break;
    }
}

int main() {
    stdio_init_all();

    // Initialize I2C slave first (GPIOs 0 and 1)
    i2c_slave_init(CONFIG_I2C_SLAVE_ADDRESS, CONFIG_I2C_INTERRUPT_GPIO);

    // Initialize power latch and start with closed latch
    power_latch_init(CONFIG_POWER_LATCH_GPIO);
    power_latch_close();

    // Initialize power button
    button_t power_button = {0};
    button_init(&power_button, CONFIG_POWER_LATCH_GPIO, false, DEBOUNCE_MS, true, false);

    // Initialize LED controller
    led_controller_init(CONFIG_LED_GPIO);
    
    // Initialize tick service (1ms ticks)
    tick_service_init(1000);

    // Initialize switch tracker for power button logic
    switch_tracker_t tracker;
    switch_tracker_init(&tracker, STARTUP_WINDOW_MS, FIRST_PRESS_HOLD_MS, LONG_PRESS_MS);

    // Initialize key FIFO
    key_fifo_t key_fifo;
    key_fifo_init(&key_fifo);
    i2c_slave_set_fifo(&key_fifo);

    // Initialize matrix scanner
    const uint8_t row_gpios[] = {
        CONFIG_ROW_1_GPIO, CONFIG_ROW_2_GPIO, CONFIG_ROW_3_GPIO,
        CONFIG_ROW_4_GPIO, CONFIG_ROW_5_GPIO, CONFIG_ROW_6_GPIO
    };
    const uint8_t col_gpios[] = {
        CONFIG_COL_A_GPIO, CONFIG_COL_B_GPIO, CONFIG_COL_C_GPIO,
        CONFIG_COL_D_GPIO, CONFIG_COL_E_GPIO, CONFIG_COL_F_GPIO,
        CONFIG_COL_G_GPIO
    };
    matrix_scanner_t matrix_scanner;
    matrix_scanner_init(&matrix_scanner, row_gpios, col_gpios, DEBOUNCE_MS);

    // Initialize FN keys
    const uint8_t fn_gpios[] = {
        CONFIG_FN1_GPIO, CONFIG_FN2_GPIO, CONFIG_FN3_GPIO, CONFIG_FN4_GPIO,
        CONFIG_FN5_GPIO, CONFIG_FN6_GPIO, CONFIG_FN8_GPIO, CONFIG_FN9_GPIO,
        CONFIG_FN10_GPIO, CONFIG_FN11_GPIO, CONFIG_FN12_GPIO
    };
    fn_keys_t fn_keys;
    fn_keys_init(&fn_keys, fn_gpios, DEBOUNCE_MS);

    // Initialize modifier manager
    modifier_manager_t modifier_manager;
    uint8_t fn_key_code = matrix_get_key_code(MODIFIER_FN_ROW, MODIFIER_FN_COL);
    uint8_t alt_key_code = matrix_get_key_code(MODIFIER_ALT_ROW, MODIFIER_ALT_COL);
    uint8_t shift_key_code = matrix_get_key_code(MODIFIER_SHIFT_ROW, MODIFIER_SHIFT_COL);
    modifier_manager_init(&modifier_manager, fn_key_code, alt_key_code, shift_key_code,
                         MODIFIER_DOUBLE_PRESS_WINDOW_MS);

    // Initialize digital mouse
    digital_mouse_t digital_mouse;
    digital_mouse_init(&digital_mouse, MOUSE_UPDATE_INTERVAL_MS);

    // Track previous states for interrupt generation
    bool prev_power_pressed = false;
    uint8_t prev_modifier_mask = 0;

    while (true) {
        if (tick_consume()) {
            uint32_t now_ms = tick_now_ms();

            // Update power button
            button_update(&power_button, now_ms);
            bool power_pressed = button_is_pressed(&power_button);

            // Set power button interrupt flag on state change
            if (power_pressed != prev_power_pressed) {
                i2c_slave_set_interrupt_flags(I2C_INT_POWER_BUTTON);
                prev_power_pressed = power_pressed;
            }

            // Update LED for power button state
            led_controller_set_power_pressed(power_pressed);

            // Process power button switch tracking
            switch_event_t event = switch_tracker_tick(&tracker, power_pressed, now_ms);
            process_switch_event(event, now_ms);

            // Scan matrix keyboard
            matrix_scanner_tick(&matrix_scanner, now_ms);

            // Scan FN keys
            fn_keys_tick(&fn_keys, now_ms);

            // Process matrix events
            key_event_t matrix_event;
            bool had_matrix_event = false;
            while (matrix_scanner_get_event(&matrix_scanner, &matrix_event)) {
                had_matrix_event = true;
                bool is_modifier = false;

                // Check if this is a modifier key
                if (matrix_event.type == KEY_EVENT_PRESS) {
                    is_modifier = modifier_manager_on_key_press(&modifier_manager, 
                                                                matrix_event.key_code, now_ms);
                } else if (matrix_event.type == KEY_EVENT_RELEASE) {
                    is_modifier = modifier_manager_on_key_release(&modifier_manager, 
                                                                  matrix_event.key_code, now_ms);
                }

                // If not a modifier, notify modifier manager of other key press
                if (!is_modifier && matrix_event.type == KEY_EVENT_PRESS) {
                    modifier_manager_on_other_key_press(&modifier_manager);
                }

                // Push event to FIFO
                key_fifo_push(&key_fifo, matrix_event.type, matrix_event.key_code);
            }
            
            // Set key event interrupt flag if any matrix events occurred
            if (had_matrix_event) {
                i2c_slave_set_interrupt_flags(I2C_INT_KEY_EVENT);
            }

            // Process FN key events
            fn_event_t fn_event;
            bool had_mouse_event = false;
            bool had_fn_keyboard_event = false;
            while (fn_keys_get_event(&fn_keys, &fn_event)) {
                uint8_t fn_index = fn_event.key_code - FN_KEY_CODE_BASE;
                
                // FN9-FN12 control mouse movement (don't go to FIFO)
                if (fn_index >= FN_KEY_FN9 && fn_index <= FN_KEY_FN12) {
                    bool pressed = (fn_event.type == FN_EVENT_PRESS || fn_event.type == FN_EVENT_HOLD);
                    digital_mouse_update_button(&digital_mouse, fn_index, pressed);
                    had_mouse_event = true;
                    // Don't push mouse movement keys to FIFO
                    continue;
                }
                
                // FN1-FN6 and FN8 are keyboard/action keys
                had_fn_keyboard_event = true;  // Track that we have FN keyboard events
                
                // Notify modifier manager that a non-modifier key was pressed (deactivates sticky modifiers)
                if (fn_event.type == FN_EVENT_PRESS) {
                    modifier_manager_on_other_key_press(&modifier_manager);
                }
                
                // Push to FIFO
                key_fifo_push(&key_fifo, fn_event.type, fn_event.key_code);
            }
            
            // Set key event interrupt flag for all FN keyboard events (press, hold, release)
            if (had_fn_keyboard_event) {
                i2c_slave_set_interrupt_flags(I2C_INT_KEY_EVENT);
            }

            // Update digital mouse position
            digital_mouse_tick(&digital_mouse, now_ms);

            // Update I2C registers
            uint8_t modifier_mask = modifier_manager_get_active_mask(&modifier_manager);
            i2c_slave_update_modifiers(modifier_mask);
            
            // Check for modifier changes and set appropriate interrupt flags
            if (modifier_mask != prev_modifier_mask) {
                uint8_t changed = modifier_mask ^ prev_modifier_mask;
                if (changed & 0x01) i2c_slave_set_interrupt_flags(I2C_INT_FN_MOD);
                if (changed & 0x02) i2c_slave_set_interrupt_flags(I2C_INT_ALT_MOD);
                if (changed & 0x04) i2c_slave_set_interrupt_flags(I2C_INT_SHIFT_MOD);
                prev_modifier_mask = modifier_mask;
            }

            int8_t mouse_x = digital_mouse_get_and_clear_x(&digital_mouse);
            int8_t mouse_y = digital_mouse_get_and_clear_y(&digital_mouse);
            i2c_slave_update_mouse(mouse_x, mouse_y);
            
            // Set mouse interrupt flag if movement occurred
            if (had_mouse_event || mouse_x != 0 || mouse_y != 0) {
                i2c_slave_set_interrupt_flags(I2C_INT_MOUSE_EVENT);
            }
            
            // Check for FIFO overflow and set interrupt flag
            if (key_fifo_check_and_clear_overflow(&key_fifo)) {
                i2c_slave_set_interrupt_flags(I2C_INT_FIFO_OVERFLOW);
            }

            // Notify I2C if events are available
            if (!key_fifo_is_empty(&key_fifo)) {
                i2c_slave_notify_events_available();
            } else {
                i2c_slave_check_and_clear_interrupt();
            }

            // Update LED controller based on active modifier
            int8_t active_mod = modifier_manager_get_active_for_led(&modifier_manager);
            led_controller_set_modifier(active_mod);
            led_controller_tick(now_ms);
        }

        tight_loop_contents();
    }

    return 0;
}
