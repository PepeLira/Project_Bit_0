#include "led.h"

#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "../build/ws2812.pio.h"

#define WS2812_SM 0
#define WS2812_FREQ 800000
#define WS2812_IS_RGBW false

static PIO led_pio = pio0;
static uint32_t led_pin = 28;
static uint32_t led_offset = 0;

static inline uint32_t pack_grb(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(g) << 16) | ((uint32_t)(r) << 8) | b;
}

void led_init(uint32_t pin) {
    led_pin = pin;
    led_pio = pio0;
    led_offset = pio_add_program(led_pio, &ws2812_program);
    ws2812_program_init(led_pio, WS2812_SM, led_offset, led_pin, WS2812_FREQ, WS2812_IS_RGBW);
}

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    pio_sm_put_blocking(led_pio, WS2812_SM, pack_grb(r, g, b) << 8u);
}
