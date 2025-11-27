#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include <stdbool.h>
#include <stdint.h>

void led_controller_init(uint32_t led_pin);
void led_controller_set_power_pressed(bool pressed);
void led_controller_set_modifier(int8_t modifier_index);  // -1 for none, 0-2 for FN/ALT/SHIFT
void led_controller_pulse_short_press(uint32_t now_ms);
void led_controller_tick(uint32_t now_ms);

#endif  // LED_CONTROLLER_H
