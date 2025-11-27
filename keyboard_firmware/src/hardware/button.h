#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

typedef struct {
    uint32_t pin;
    bool active_high;
    uint32_t debounce_ms;

    bool stable_state;
    bool last_read;
    uint32_t debounce_start_ms;
    bool debouncing;
} button_t;

void button_init(button_t *button, uint32_t pin, bool active_high, uint32_t debounce_ms, bool enable_pull_up,
                 bool enable_pull_down);

bool button_update(button_t *button, uint32_t now_ms);
bool button_is_pressed(const button_t *button);

#endif  // BUTTON_H
