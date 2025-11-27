#ifndef SWITCH_TRACKER_H
#define SWITCH_TRACKER_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    SWITCH_EVENT_NONE = 0,
    SWITCH_EVENT_FIRST_PRESS,
    SWITCH_EVENT_SHORT_PRESS,
    SWITCH_EVENT_LONG_PRESS
} switch_event_t;

typedef struct {
    uint32_t startup_window_ms;
    uint32_t first_press_hold_ms;
    uint32_t long_press_ms;

    bool first_press_latched;
    bool first_press_in_progress;
    bool startup_window_elapsed;

    bool press_active;
    uint32_t press_start_ms;
    bool suppress_long_for_press;
    bool long_press_emitted;
} switch_tracker_t;

void switch_tracker_init(switch_tracker_t *tracker, uint32_t startup_window_ms, uint32_t first_press_hold_ms,
                         uint32_t long_press_ms);

switch_event_t switch_tracker_tick(switch_tracker_t *tracker, bool pressed, uint32_t now_ms);

bool switch_tracker_should_hold_latch(const switch_tracker_t *tracker);
bool switch_tracker_startup_elapsed(const switch_tracker_t *tracker);
bool switch_tracker_first_press_pending(const switch_tracker_t *tracker);

#endif  // SWITCH_TRACKER_H
