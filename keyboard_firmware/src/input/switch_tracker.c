#include "switch_tracker.h"

#include <stddef.h>

void switch_tracker_init(switch_tracker_t *tracker, uint32_t startup_window_ms, uint32_t first_press_hold_ms,
                         uint32_t long_press_ms) {
    if (!tracker) {
        return;
    }
    tracker->startup_window_ms = startup_window_ms;
    tracker->first_press_hold_ms = first_press_hold_ms;
    tracker->long_press_ms = long_press_ms;

    tracker->first_press_latched = false;
    tracker->first_press_in_progress = false;
    tracker->startup_window_elapsed = false;
    tracker->press_active = false;
    tracker->press_start_ms = 0;
    tracker->suppress_long_for_press = false;
    tracker->long_press_emitted = false;
}

static void handle_startup_window(switch_tracker_t *tracker, uint32_t now_ms) {
    if (!tracker->startup_window_elapsed && now_ms >= tracker->startup_window_ms) {
        tracker->startup_window_elapsed = true;
    }
}

switch_event_t switch_tracker_tick(switch_tracker_t *tracker, bool pressed, uint32_t now_ms) {
    if (!tracker) {
        return SWITCH_EVENT_NONE;
    }

    handle_startup_window(tracker, now_ms);

    // Start of a press
    if (pressed && !tracker->press_active) {
        tracker->press_active = true;
        tracker->press_start_ms = now_ms;
        tracker->long_press_emitted = false;
        tracker->suppress_long_for_press = (!tracker->first_press_latched && !tracker->startup_window_elapsed);
        tracker->first_press_in_progress = tracker->suppress_long_for_press;
    }

    // While pressed, check for thresholds
    if (pressed && tracker->press_active) {
        uint32_t held_ms = now_ms - tracker->press_start_ms;
        if (tracker->first_press_in_progress && held_ms >= tracker->first_press_hold_ms) {
            tracker->first_press_latched = true;
            tracker->first_press_in_progress = false;
            return SWITCH_EVENT_FIRST_PRESS;
        }
        if (!tracker->suppress_long_for_press && !tracker->long_press_emitted && held_ms >= tracker->long_press_ms) {
            tracker->long_press_emitted = true;
            return SWITCH_EVENT_LONG_PRESS;
        }
    }

    // Release
    if (!pressed && tracker->press_active) {
        tracker->press_active = false;
        uint32_t held_ms = now_ms - tracker->press_start_ms;

        if (tracker->first_press_in_progress) {
            tracker->first_press_in_progress = false;
        }

        if (tracker->suppress_long_for_press) {
            // first press path, already handled separately
            return SWITCH_EVENT_NONE;
        }

        if (!tracker->long_press_emitted && held_ms >= tracker->first_press_hold_ms) {
            if (held_ms >= tracker->long_press_ms) {
                return SWITCH_EVENT_LONG_PRESS;
            }
            return SWITCH_EVENT_SHORT_PRESS;
        }
    }

    return SWITCH_EVENT_NONE;
}

bool switch_tracker_should_hold_latch(const switch_tracker_t *tracker) {
    return tracker && tracker->first_press_latched;
}

bool switch_tracker_startup_elapsed(const switch_tracker_t *tracker) {
    return tracker && tracker->startup_window_elapsed;
}

bool switch_tracker_first_press_pending(const switch_tracker_t *tracker) {
    return tracker && tracker->first_press_in_progress;
}
