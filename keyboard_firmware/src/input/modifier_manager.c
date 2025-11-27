#include "modifier_manager.h"
#include <string.h>

void modifier_manager_init(modifier_manager_t *manager, uint8_t fn_key_code, 
                          uint8_t alt_key_code, uint8_t shift_key_code,
                          uint32_t double_press_window_ms) {
    memset(manager, 0, sizeof(modifier_manager_t));
    
    // Initialize FN modifier
    manager->modifiers[MODIFIER_FN].key_code = fn_key_code;
    manager->modifiers[MODIFIER_FN].double_press_window_ms = double_press_window_ms;
    
    // Initialize ALT modifier
    manager->modifiers[MODIFIER_ALT].key_code = alt_key_code;
    manager->modifiers[MODIFIER_ALT].double_press_window_ms = double_press_window_ms;
    
    // Initialize SHIFT modifier
    manager->modifiers[MODIFIER_SHIFT].key_code = shift_key_code;
    manager->modifiers[MODIFIER_SHIFT].double_press_window_ms = double_press_window_ms;
    
    manager->active_modifier_mask = 0;
}

// Helper to find modifier by key code
static int8_t find_modifier_index(const modifier_manager_t *manager, uint8_t key_code) {
    for (int i = 0; i < MODIFIER_COUNT; i++) {
        if (manager->modifiers[i].key_code == key_code) {
            return i;
        }
    }
    return -1;
}

bool modifier_manager_on_key_press(modifier_manager_t *manager, uint8_t key_code, uint32_t now_ms) {
    int8_t idx = find_modifier_index(manager, key_code);
    if (idx < 0) {
        return false;  // Not a modifier key
    }
    
    modifier_info_t *mod = &manager->modifiers[idx];
    mod->press_pending = true;
    
    return true;  // This is a modifier key
}

bool modifier_manager_on_key_release(modifier_manager_t *manager, uint8_t key_code, uint32_t now_ms) {
    int8_t idx = find_modifier_index(manager, key_code);
    if (idx < 0) {
        return false;  // Not a modifier key
    }
    
    modifier_info_t *mod = &manager->modifiers[idx];
    
    if (!mod->press_pending) {
        return true;  // Already processed or spurious release
    }
    
    mod->press_pending = false;
    
    // Check if this is a double press (within time window)
    bool is_double_press = false;
    if ((now_ms - mod->last_press_time) <= mod->double_press_window_ms) {
        is_double_press = true;
    }
    mod->last_press_time = now_ms;
    
    // Update modifier state based on current state and press type
    uint8_t mask = (1 << idx);
    
    switch (mod->state) {
        case MODIFIER_STATE_INACTIVE:
            if (is_double_press) {
                // Double press -> go to locked
                mod->state = MODIFIER_STATE_LOCKED;
                manager->active_modifier_mask |= mask;
            } else {
                // Single press -> go to sticky
                mod->state = MODIFIER_STATE_STICKY;
                manager->active_modifier_mask |= mask;
            }
            break;
            
        case MODIFIER_STATE_STICKY:
            if (is_double_press) {
                // Double press while sticky -> go to locked
                mod->state = MODIFIER_STATE_LOCKED;
                manager->active_modifier_mask |= mask;
            } else {
                // Single press while sticky -> deactivate
                mod->state = MODIFIER_STATE_INACTIVE;
                manager->active_modifier_mask &= ~mask;
            }
            break;
            
        case MODIFIER_STATE_LOCKED:
            // Any press while locked -> deactivate
            mod->state = MODIFIER_STATE_INACTIVE;
            manager->active_modifier_mask &= ~mask;
            break;
    }
    
    return true;  // This is a modifier key
}

void modifier_manager_on_other_key_press(modifier_manager_t *manager) {
    // Deactivate all sticky modifiers (but not locked ones)
    for (int i = 0; i < MODIFIER_COUNT; i++) {
        if (manager->modifiers[i].state == MODIFIER_STATE_STICKY) {
            manager->modifiers[i].state = MODIFIER_STATE_INACTIVE;
            manager->active_modifier_mask &= ~(1 << i);
        }
    }
}

bool modifier_manager_is_active(const modifier_manager_t *manager, uint8_t modifier_index) {
    if (modifier_index >= MODIFIER_COUNT) {
        return false;
    }
    return (manager->active_modifier_mask & (1 << modifier_index)) != 0;
}

uint8_t modifier_manager_get_active_mask(const modifier_manager_t *manager) {
    return manager->active_modifier_mask;
}

int8_t modifier_manager_get_active_for_led(const modifier_manager_t *manager) {
    // Priority: FN > ALT > SHIFT
    if (modifier_manager_is_active(manager, MODIFIER_FN)) {
        return MODIFIER_FN;
    }
    if (modifier_manager_is_active(manager, MODIFIER_ALT)) {
        return MODIFIER_ALT;
    }
    if (modifier_manager_is_active(manager, MODIFIER_SHIFT)) {
        return MODIFIER_SHIFT;
    }
    return -1;  // No modifier active
}
