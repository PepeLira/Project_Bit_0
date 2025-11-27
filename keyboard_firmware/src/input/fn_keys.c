#include "fn_keys.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

// Event queue for FN key events
#define MAX_FN_EVENTS 16
static fn_event_t fn_event_queue[MAX_FN_EVENTS];
static uint8_t fn_event_queue_head = 0;
static uint8_t fn_event_queue_tail = 0;
static uint8_t fn_event_queue_count = 0;

// Helper to add event to queue
static bool queue_fn_event(fn_event_type_t type, uint8_t key_code) {
    if (fn_event_queue_count >= MAX_FN_EVENTS) {
        return false;  // Queue full
    }
    
    fn_event_queue[fn_event_queue_tail].type = type;
    fn_event_queue[fn_event_queue_tail].key_code = key_code;
    fn_event_queue_tail = (fn_event_queue_tail + 1) % MAX_FN_EVENTS;
    fn_event_queue_count++;
    
    return true;
}

void fn_keys_init(fn_keys_t *fn_keys, const uint8_t *gpios, uint32_t debounce_ms) {
    // Copy GPIO array
    memcpy(fn_keys->gpios, gpios, FN_KEY_COUNT);
    fn_keys->debounce_ms = debounce_ms;
    
    // Initialize state arrays
    memset(fn_keys->keys, 0, sizeof(fn_keys->keys));
    
    // Configure all FN key GPIOs as inputs with pull-ups
    for (int i = 0; i < FN_KEY_COUNT; i++) {
        gpio_init(gpios[i]);
        gpio_set_dir(gpios[i], GPIO_IN);
        gpio_pull_up(gpios[i]);
    }
    
    // Clear event queue
    fn_event_queue_head = 0;
    fn_event_queue_tail = 0;
    fn_event_queue_count = 0;
}

void fn_keys_tick(fn_keys_t *fn_keys, uint32_t now_ms) {
    for (int i = 0; i < FN_KEY_COUNT; i++) {
        fn_key_state_t *key = &fn_keys->keys[i];
        
        // Read current state (active low)
        bool pressed = !gpio_get(fn_keys->gpios[i]);
        key->current_state = pressed;
        
        // Debounce logic
        if (pressed != key->previous_state) {
            // State changed, reset timer
            key->state_time = now_ms;
            key->previous_state = pressed;
        } else if ((now_ms - key->state_time) >= fn_keys->debounce_ms) {
            // State stable for debounce period
            bool old_debounced = key->debounced_state;
            key->debounced_state = pressed;
            
            // Generate events on debounced state changes
            uint8_t key_code = fn_keys_get_key_code(i);
            
            if (pressed && !old_debounced) {
                // Key press
                queue_fn_event(FN_EVENT_PRESS, key_code);
                key->hold_emitted = false;
            } else if (!pressed && old_debounced) {
                // Key release
                queue_fn_event(FN_EVENT_RELEASE, key_code);
                key->hold_emitted = false;
            } else if (pressed && old_debounced && !key->hold_emitted) {
                // Check for hold event (key held for longer period)
                if ((now_ms - key->state_time) >= 500) {  // 500ms hold threshold
                    queue_fn_event(FN_EVENT_HOLD, key_code);
                    key->hold_emitted = true;
                }
            }
        }
    }
}

bool fn_keys_get_event(fn_keys_t *fn_keys, fn_event_t *event) {
    if (fn_event_queue_count == 0) {
        return false;
    }
    
    *event = fn_event_queue[fn_event_queue_head];
    fn_event_queue_head = (fn_event_queue_head + 1) % MAX_FN_EVENTS;
    fn_event_queue_count--;
    
    return true;
}

bool fn_keys_is_pressed(const fn_keys_t *fn_keys, uint8_t key_index) {
    if (key_index >= FN_KEY_COUNT) {
        return false;
    }
    return fn_keys->keys[key_index].debounced_state;
}
