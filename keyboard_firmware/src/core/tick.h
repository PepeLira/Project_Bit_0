#ifndef TICK_H
#define TICK_H

#include <stdbool.h>
#include <stdint.h>

void tick_service_init(uint32_t interval_us);
bool tick_consume(void);
uint32_t tick_now_ms(void);

#endif  // TICK_H
