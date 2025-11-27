#include "tick.h"

#include "hardware/timer.h"
#include "pico/stdlib.h"

static repeating_timer_t tick_timer;
static volatile bool tick_flag = false;

static bool tick_callback(repeating_timer_t *rt) {
    (void)rt;
    tick_flag = true;
    return true;
}

void tick_service_init(uint32_t interval_us) {
    tick_flag = false;
    add_repeating_timer_us(-((int64_t)interval_us), tick_callback, NULL, &tick_timer);
}

bool tick_consume(void) {
    if (tick_flag) {
        tick_flag = false;
        return true;
    }
    return false;
}

uint32_t tick_now_ms(void) {
    return to_ms_since_boot(get_absolute_time());
}
