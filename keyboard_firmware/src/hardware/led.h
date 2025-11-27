#ifndef LED_H
#define LED_H

#include <stdint.h>

void led_init(uint32_t pin);
void led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

#endif  // LED_H
