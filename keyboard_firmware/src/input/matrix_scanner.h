#ifndef MATRIX_SCANNER_H
#define MATRIX_SCANNER_H

#include <stdbool.h>
#include <stdint.h>

// Matrix dimensions
#define MATRIX_ROWS 6
#define MATRIX_COLS 7

// Key event types
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESS = 1,
    KEY_EVENT_HOLD = 2,
    KEY_EVENT_RELEASE = 3
} key_event_type_t;

// Key event structure
typedef struct {
    key_event_type_t type;
    uint8_t key_code;  // Row * MATRIX_COLS + Col
} key_event_t;

// Matrix scanner state
typedef struct {
    uint8_t row_gpios[MATRIX_ROWS];
    uint8_t col_gpios[MATRIX_COLS];
    uint32_t debounce_ms;
    
    // Per-key state
    bool current_state[MATRIX_ROWS][MATRIX_COLS];
    bool previous_state[MATRIX_ROWS][MATRIX_COLS];
    uint32_t state_time[MATRIX_ROWS][MATRIX_COLS];
    bool debounced_state[MATRIX_ROWS][MATRIX_COLS];
    bool hold_emitted[MATRIX_ROWS][MATRIX_COLS];
} matrix_scanner_t;

/**
 * Initialize the matrix scanner.
 * 
 * @param scanner Pointer to scanner state
 * @param row_gpios Array of row GPIO numbers
 * @param col_gpios Array of column GPIO numbers
 * @param debounce_ms Debounce time in milliseconds
 */
void matrix_scanner_init(matrix_scanner_t *scanner, const uint8_t *row_gpios, 
                        const uint8_t *col_gpios, uint32_t debounce_ms);

/**
 * Scan the matrix and update internal state.
 * Must be called regularly (e.g., every 1ms).
 * 
 * @param scanner Pointer to scanner state
 * @param now_ms Current time in milliseconds
 */
void matrix_scanner_tick(matrix_scanner_t *scanner, uint32_t now_ms);

/**
 * Get the next pending key event.
 * 
 * @param scanner Pointer to scanner state
 * @param event Output event structure
 * @return true if an event was retrieved, false if no events pending
 */
bool matrix_scanner_get_event(matrix_scanner_t *scanner, key_event_t *event);

/**
 * Check if a specific key is currently pressed (debounced).
 * 
 * @param scanner Pointer to scanner state
 * @param row Row index (0-5)
 * @param col Column index (0-6)
 * @return true if key is pressed
 */
bool matrix_scanner_is_key_pressed(const matrix_scanner_t *scanner, uint8_t row, uint8_t col);

/**
 * Get key code from row and column.
 * 
 * @param row Row index (0-5)
 * @param col Column index (0-6)
 * @return Key code
 */
static inline uint8_t matrix_get_key_code(uint8_t row, uint8_t col) {
    return row * MATRIX_COLS + col;
}

#endif  // MATRIX_SCANNER_H
