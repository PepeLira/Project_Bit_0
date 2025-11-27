#ifndef FN_KEYS_H
#define FN_KEYS_H

#include <stdbool.h>
#include <stdint.h>

// Number of independent FN keys (FN1-FN6, FN8-FN12 = 11 keys)
#define FN_KEY_COUNT 11

// FN key indices
#define FN_KEY_FN1  0
#define FN_KEY_FN2  1
#define FN_KEY_FN3  2
#define FN_KEY_FN4  3
#define FN_KEY_FN5  4
#define FN_KEY_FN6  5
#define FN_KEY_FN8  6
#define FN_KEY_FN9  7
#define FN_KEY_FN10 8
#define FN_KEY_FN11 9
#define FN_KEY_FN12 10

// Key codes for FN keys (start after matrix keys)
#define FN_KEY_CODE_BASE 42  // 6 rows * 7 cols = 42

// Key event types (same as matrix scanner)
typedef enum {
    FN_EVENT_NONE = 0,
    FN_EVENT_PRESS = 1,
    FN_EVENT_HOLD = 2,
    FN_EVENT_RELEASE = 3
} fn_event_type_t;

// FN key event structure
typedef struct {
    fn_event_type_t type;
    uint8_t key_code;
} fn_event_t;

// Per-key state
typedef struct {
    bool current_state;
    bool previous_state;
    uint32_t state_time;
    bool debounced_state;
    bool hold_emitted;
} fn_key_state_t;

// FN keys manager state
typedef struct {
    uint8_t gpios[FN_KEY_COUNT];
    uint32_t debounce_ms;
    fn_key_state_t keys[FN_KEY_COUNT];
} fn_keys_t;

/**
 * Initialize the FN keys manager.
 * 
 * @param fn_keys Pointer to FN keys state
 * @param gpios Array of GPIO numbers for FN keys (in order FN1-FN6, FN8-FN12)
 * @param debounce_ms Debounce time in milliseconds
 */
void fn_keys_init(fn_keys_t *fn_keys, const uint8_t *gpios, uint32_t debounce_ms);

/**
 * Update FN keys state and process events.
 * Must be called regularly (e.g., every 1ms).
 * 
 * @param fn_keys Pointer to FN keys state
 * @param now_ms Current time in milliseconds
 */
void fn_keys_tick(fn_keys_t *fn_keys, uint32_t now_ms);

/**
 * Get the next pending FN key event.
 * 
 * @param fn_keys Pointer to FN keys state
 * @param event Output event structure
 * @return true if an event was retrieved, false if no events pending
 */
bool fn_keys_get_event(fn_keys_t *fn_keys, fn_event_t *event);

/**
 * Check if a specific FN key is currently pressed (debounced).
 * 
 * @param fn_keys Pointer to FN keys state
 * @param key_index FN key index (0-10)
 * @return true if key is pressed
 */
bool fn_keys_is_pressed(const fn_keys_t *fn_keys, uint8_t key_index);

/**
 * Get key code for an FN key index.
 * 
 * @param key_index FN key index (0-10)
 * @return Key code
 */
static inline uint8_t fn_keys_get_key_code(uint8_t key_index) {
    return FN_KEY_CODE_BASE + key_index;
}

#endif  // FN_KEYS_H
