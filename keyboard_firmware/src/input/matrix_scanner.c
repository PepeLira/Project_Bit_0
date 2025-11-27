#include "matrix_scanner.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <string.h>

// Event queue for pending events
#define MAX_PENDING_EVENTS 32
static key_event_t event_queue[MAX_PENDING_EVENTS];
static uint8_t event_queue_head = 0;
static uint8_t event_queue_tail = 0;
static uint8_t event_queue_count = 0;

// Helper to add event to queue
static bool queue_event(key_event_type_t type, uint8_t key_code) {
    if (event_queue_count >= MAX_PENDING_EVENTS) {
        return false;  // Queue full
    }
    
    event_queue[event_queue_tail].type = type;
    event_queue[event_queue_tail].key_code = key_code;
    event_queue_tail = (event_queue_tail + 1) % MAX_PENDING_EVENTS;
    event_queue_count++;
    
    return true;
}

void matrix_scanner_init(matrix_scanner_t *scanner, const uint8_t *row_gpios, 
                        const uint8_t *col_gpios, uint32_t debounce_ms) {
    // Copy GPIO arrays
    memcpy(scanner->row_gpios, row_gpios, MATRIX_ROWS);
    memcpy(scanner->col_gpios, col_gpios, MATRIX_COLS);
    scanner->debounce_ms = debounce_ms;
    
    // Initialize state arrays
    memset(scanner->current_state, 0, sizeof(scanner->current_state));
    memset(scanner->previous_state, 0, sizeof(scanner->previous_state));
    memset(scanner->state_time, 0, sizeof(scanner->state_time));
    memset(scanner->debounced_state, 0, sizeof(scanner->debounced_state));
    memset(scanner->hold_emitted, 0, sizeof(scanner->hold_emitted));
    
    // Configure column GPIOs as outputs (drive low when scanning)
    for (int col = 0; col < MATRIX_COLS; col++) {
        gpio_init(col_gpios[col]);
        gpio_set_dir(col_gpios[col], GPIO_OUT);
        gpio_put(col_gpios[col], 1);  // Set high (inactive)
    }
    
    // Configure row GPIOs as inputs with pull-ups
    for (int row = 0; row < MATRIX_ROWS; row++) {
        gpio_init(row_gpios[row]);
        gpio_set_dir(row_gpios[row], GPIO_IN);
        gpio_pull_up(row_gpios[row]);
    }
    
    // Clear event queue
    event_queue_head = 0;
    event_queue_tail = 0;
    event_queue_count = 0;
}

void matrix_scanner_tick(matrix_scanner_t *scanner, uint32_t now_ms) {
    // Scan each column
    for (int col = 0; col < MATRIX_COLS; col++) {
        // Activate this column (drive low)
        gpio_put(scanner->col_gpios[col], 0);
        
        // Small delay to let signals settle
        busy_wait_us(1);
        
        // Read all rows
        for (int row = 0; row < MATRIX_ROWS; row++) {
            bool pressed = !gpio_get(scanner->row_gpios[row]);  // Active low
            
            scanner->current_state[row][col] = pressed;
            
            // Debounce logic
            if (pressed != scanner->previous_state[row][col]) {
                // State changed, reset timer
                scanner->state_time[row][col] = now_ms;
                scanner->previous_state[row][col] = pressed;
            } else if ((now_ms - scanner->state_time[row][col]) >= scanner->debounce_ms) {
                // State stable for debounce period
                bool old_debounced = scanner->debounced_state[row][col];
                scanner->debounced_state[row][col] = pressed;
                
                // Generate events on debounced state changes
                if (pressed && !old_debounced) {
                    // Key press
                    uint8_t key_code = matrix_get_key_code(row, col);
                    queue_event(KEY_EVENT_PRESS, key_code);
                    scanner->hold_emitted[row][col] = false;
                } else if (!pressed && old_debounced) {
                    // Key release
                    uint8_t key_code = matrix_get_key_code(row, col);
                    queue_event(KEY_EVENT_RELEASE, key_code);
                    scanner->hold_emitted[row][col] = false;
                } else if (pressed && old_debounced && !scanner->hold_emitted[row][col]) {
                    // Check for hold event (key held for longer period)
                    if ((now_ms - scanner->state_time[row][col]) >= 500) {  // 500ms hold threshold
                        uint8_t key_code = matrix_get_key_code(row, col);
                        queue_event(KEY_EVENT_HOLD, key_code);
                        scanner->hold_emitted[row][col] = true;
                    }
                }
            }
        }
        
        // Deactivate this column (drive high)
        gpio_put(scanner->col_gpios[col], 1);
    }
}

bool matrix_scanner_get_event(matrix_scanner_t *scanner, key_event_t *event) {
    if (event_queue_count == 0) {
        return false;
    }
    
    *event = event_queue[event_queue_head];
    event_queue_head = (event_queue_head + 1) % MAX_PENDING_EVENTS;
    event_queue_count--;
    
    return true;
}

bool matrix_scanner_is_key_pressed(const matrix_scanner_t *scanner, uint8_t row, uint8_t col) {
    if (row >= MATRIX_ROWS || col >= MATRIX_COLS) {
        return false;
    }
    return scanner->debounced_state[row][col];
}
