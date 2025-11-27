#include "button.h"

#include "hardware/gpio.h"

void button_init(button_t *button, uint32_t pin, bool active_high, uint32_t debounce_ms, bool enable_pull_up,
                 bool enable_pull_down) {
    if (!button) {
        return;
    }

    button->pin = pin;
    button->active_high = active_high;
    button->debounce_ms = debounce_ms;
    button->stable_state = false;
    button->last_read = false;
    button->debounce_start_ms = 0;
    button->debouncing = false;

    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    if (enable_pull_up) {
        gpio_pull_up(pin);
    } else {
        gpio_disable_pulls(pin);
    }
    if (enable_pull_down) {
        gpio_pull_down(pin);
    }
}

static bool normalize(bool raw, bool active_high) {
    return active_high ? raw : !raw;
}

bool button_update(button_t *button, uint32_t now_ms) {
    if (!button) {
        return false;
    }

    bool raw = gpio_get(button->pin);
    bool pressed = normalize(raw, button->active_high);

    if (pressed != button->stable_state) {
        if (!button->debouncing) {
            button->debouncing = true;
            button->debounce_start_ms = now_ms;
            button->last_read = pressed;
        } else if (button->last_read == pressed && (now_ms - button->debounce_start_ms) >= button->debounce_ms) {
            button->stable_state = pressed;
            button->debouncing = false;
            return true;
        }
    } else {
        button->debouncing = false;
    }

    return false;
}

bool button_is_pressed(const button_t *button) {
    return button && button->stable_state;
}
