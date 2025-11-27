#include "power_latch.h"

#include "hardware/gpio.h"

static uint32_t latch_pin = 29;
static bool latch_closed = false;

static void configure_default_input(void) {
    gpio_init(latch_pin);
    gpio_set_dir(latch_pin, GPIO_IN);
    gpio_disable_pulls(latch_pin);
}

void power_latch_init(uint32_t pin) {
    latch_pin = pin;
    gpio_init(latch_pin);
    gpio_set_dir(latch_pin, GPIO_IN);
    gpio_pull_up(latch_pin);
    latch_closed = true;
}

void power_latch_close(void) {
    gpio_init(latch_pin);
    gpio_set_dir(latch_pin, GPIO_IN);
    gpio_pull_up(latch_pin);
    latch_closed = true;
}

void power_latch_open(void) {
    configure_default_input();
    latch_closed = false;
}

bool power_latch_is_closed(void) {
    return latch_closed;
}
