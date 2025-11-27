#ifndef POWER_LATCH_H
#define POWER_LATCH_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

void power_latch_init(uint32_t pin);
void power_latch_close(void);
void power_latch_open(void);
bool power_latch_is_closed(void);

#endif  // POWER_LATCH_H
