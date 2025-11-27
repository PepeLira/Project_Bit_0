#ifndef MODIFIER_MANAGER_H
#define MODIFIER_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

// Number of modifiers (FN, ALT, LSHIFT)
#define MODIFIER_COUNT 3

// Modifier indices
#define MODIFIER_FN     0  // C6
#define MODIFIER_ALT    1  // C5
#define MODIFIER_SHIFT  2  // E4

// Modifier states
typedef enum {
    MODIFIER_STATE_INACTIVE = 0,
    MODIFIER_STATE_STICKY,    // Active until another non-modifier key is pressed
    MODIFIER_STATE_LOCKED     // Locked until modifier pressed again
} modifier_state_t;

// Per-modifier state
typedef struct {
    modifier_state_t state;
    uint8_t key_code;           // Key code that activates this modifier
    uint32_t last_press_time;   // Time of last press (for double-press detection)
    bool press_pending;         // Waiting for release to process
    uint32_t double_press_window_ms;  // Time window for double press detection
} modifier_info_t;

// Modifier manager state
typedef struct {
    modifier_info_t modifiers[MODIFIER_COUNT];
    uint8_t active_modifier_mask;  // Bitmask of currently active modifiers
} modifier_manager_t;

/**
 * Initialize the modifier manager.
 * 
 * @param manager Pointer to modifier manager state
 * @param fn_key_code Key code for FN modifier (C6)
 * @param alt_key_code Key code for ALT modifier (C5)
 * @param shift_key_code Key code for SHIFT modifier (E4)
 * @param double_press_window_ms Time window for double-press detection (e.g., 300ms)
 */
void modifier_manager_init(modifier_manager_t *manager, uint8_t fn_key_code, 
                          uint8_t alt_key_code, uint8_t shift_key_code,
                          uint32_t double_press_window_ms);

/**
 * Process a key press event.
 * 
 * @param manager Pointer to modifier manager state
 * @param key_code Key code that was pressed
 * @param now_ms Current time in milliseconds
 * @return true if this was a modifier key, false otherwise
 */
bool modifier_manager_on_key_press(modifier_manager_t *manager, uint8_t key_code, uint32_t now_ms);

/**
 * Process a key release event.
 * 
 * @param manager Pointer to modifier manager state
 * @param key_code Key code that was released
 * @param now_ms Current time in milliseconds
 * @return true if this was a modifier key, false otherwise
 */
bool modifier_manager_on_key_release(modifier_manager_t *manager, uint8_t key_code, uint32_t now_ms);

/**
 * Notify that a non-modifier key was pressed.
 * This deactivates sticky modifiers.
 * 
 * @param manager Pointer to modifier manager state
 */
void modifier_manager_on_other_key_press(modifier_manager_t *manager);

/**
 * Check if a specific modifier is active.
 * 
 * @param manager Pointer to modifier manager state
 * @param modifier_index Modifier index (MODIFIER_FN, MODIFIER_ALT, MODIFIER_SHIFT)
 * @return true if modifier is active
 */
bool modifier_manager_is_active(const modifier_manager_t *manager, uint8_t modifier_index);

/**
 * Get the bitmask of active modifiers.
 * Bits [0:2] correspond to FN, ALT, SHIFT.
 * 
 * @param manager Pointer to modifier manager state
 * @return Bitmask of active modifiers
 */
uint8_t modifier_manager_get_active_mask(const modifier_manager_t *manager);

/**
 * Get the currently active modifier for LED display.
 * Returns the index of the highest priority active modifier, or -1 if none active.
 * Priority: FN > ALT > SHIFT
 * 
 * @param manager Pointer to modifier manager state
 * @return Modifier index or -1 if none active
 */
int8_t modifier_manager_get_active_for_led(const modifier_manager_t *manager);

#endif  // MODIFIER_MANAGER_H
