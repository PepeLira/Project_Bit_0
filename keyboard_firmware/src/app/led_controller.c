#include "led_controller.h"

#include <stdbool.h>
#include <stdint.h>

#include "../config/config.h"
#include "../hardware/led.h"

#define BLINK_INTERVAL_MS 500
#define PULSE_DURATION_MS 200

static bool power_pressed = false;
static int8_t active_modifier = -1;  // -1=none, 0=FN, 1=ALT, 2=SHIFT
static bool pulse_active = false;
static uint32_t pulse_end_ms = 0;
static bool blink_on = false;
static uint32_t next_blink_toggle_ms = 0;

static inline uint8_t color_r(uint32_t color) { return (color >> 16) & 0xFF; }
static inline uint8_t color_g(uint32_t color) { return (color >> 8) & 0xFF; }
static inline uint8_t color_b(uint32_t color) { return color & 0xFF; }

static void set_color(uint32_t color) {
    led_set_rgb(color_r(color), color_g(color), color_b(color));
}

static void set_idle(void) {
    set_color(CONFIG_COLOR_IDLE);
}

static void set_modifier(int8_t modifier_index) {
    uint32_t color;
    switch (modifier_index) {
        case 0:  // FN
            color = CONFIG_COLOR_MOD_FN;
            break;
        case 1:  // ALT
            color = CONFIG_COLOR_MOD_ALT;
            break;
        case 2:  // SHIFT
            color = CONFIG_COLOR_MOD_SHIFT;
            break;
        default:
            color = CONFIG_COLOR_IDLE;
            break;
    }
    set_color(color);
}

static void set_power(bool on) {
    if (on) {
        set_color(CONFIG_COLOR_POWER);
    } else {
        set_idle();
    }
}

static void set_pulse(void) {
    set_color(CONFIG_COLOR_PULSE);
}

static void refresh(uint32_t now_ms) {
    // Priority: Power press > Modifier > Pulse > Idle
    if (power_pressed) {
        if (now_ms >= next_blink_toggle_ms) {
            blink_on = !blink_on;
            next_blink_toggle_ms = now_ms + BLINK_INTERVAL_MS;
        }
        set_power(blink_on);
        return;
    }

    if (active_modifier >= 0) {
        set_modifier(active_modifier);
        return;
    }

    if (pulse_active) {
        if (now_ms >= pulse_end_ms) {
            pulse_active = false;
        } else {
            set_pulse();
            return;
        }
    }

    set_idle();
}

void led_controller_init(uint32_t led_pin) {
    led_init(led_pin);
    blink_on = false;
    next_blink_toggle_ms = 0;
    power_pressed = false;
    active_modifier = -1;
    pulse_active = false;
    set_idle();
}

void led_controller_set_power_pressed(bool pressed) {
    power_pressed = pressed;
    if (pressed && next_blink_toggle_ms == 0) {
        blink_on = true;
        // Schedule first toggle in the future, not immediately
        next_blink_toggle_ms = 1;
    }
}

void led_controller_set_modifier(int8_t modifier_index) {
    active_modifier = modifier_index;
}

void led_controller_pulse_short_press(uint32_t now_ms) {
    pulse_active = true;
    pulse_end_ms = now_ms + PULSE_DURATION_MS;
}

void led_controller_tick(uint32_t now_ms) {
    refresh(now_ms);
}
