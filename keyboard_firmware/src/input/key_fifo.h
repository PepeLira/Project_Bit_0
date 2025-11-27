#ifndef KEY_FIFO_H
#define KEY_FIFO_H

#include <stdbool.h>
#include <stdint.h>

// FIFO depth
#define KEY_FIFO_SIZE 64

// Key event entry format:
// Bits [1:0]: Event type (00=none, 01=press, 10=hold, 11=release)
// Bits [7:2]: Key code (0-63 for 64 possible keys)
#define KEY_FIFO_EVENT_TYPE_MASK    0x03
#define KEY_FIFO_EVENT_TYPE_SHIFT   0
#define KEY_FIFO_KEY_CODE_MASK      0xFC
#define KEY_FIFO_KEY_CODE_SHIFT     2

#define KEY_FIFO_EVENT_NONE     0
#define KEY_FIFO_EVENT_PRESS    1
#define KEY_FIFO_EVENT_HOLD     2
#define KEY_FIFO_EVENT_RELEASE  3

// Special value for "no event" when FIFO is empty
#define KEY_FIFO_NO_EVENT 0x00

// FIFO state
typedef struct {
    uint8_t buffer[KEY_FIFO_SIZE];
    uint8_t head;   // Read position
    uint8_t tail;   // Write position
    uint8_t count;  // Number of entries
    bool overflow;  // Set when push fails due to full FIFO
} key_fifo_t;

/**
 * Initialize the key event FIFO.
 * 
 * @param fifo Pointer to FIFO state
 */
void key_fifo_init(key_fifo_t *fifo);

/**
 * Push a key event into the FIFO.
 * 
 * @param fifo Pointer to FIFO state
 * @param event_type Event type (KEY_FIFO_EVENT_PRESS, etc.)
 * @param key_code Key code (0-63)
 * @return true if event was pushed, false if FIFO is full
 */
bool key_fifo_push(key_fifo_t *fifo, uint8_t event_type, uint8_t key_code);

/**
 * Pop a key event from the FIFO.
 * 
 * @param fifo Pointer to FIFO state
 * @return Event entry, or KEY_FIFO_NO_EVENT if FIFO is empty
 */
uint8_t key_fifo_pop(key_fifo_t *fifo);

/**
 * Peek at the next event without removing it.
 * 
 * @param fifo Pointer to FIFO state
 * @return Event entry, or KEY_FIFO_NO_EVENT if FIFO is empty
 */
uint8_t key_fifo_peek(const key_fifo_t *fifo);

/**
 * Get the number of events in the FIFO.
 * 
 * @param fifo Pointer to FIFO state
 * @return Number of events (0-KEY_FIFO_SIZE)
 */
uint8_t key_fifo_count(const key_fifo_t *fifo);

/**
 * Check if the FIFO is empty.
 * 
 * @param fifo Pointer to FIFO state
 * @return true if empty
 */
bool key_fifo_is_empty(const key_fifo_t *fifo);

/**
 * Check if the FIFO is full.
 * 
 * @param fifo Pointer to FIFO state
 * @return true if full
 */
bool key_fifo_is_full(const key_fifo_t *fifo);

/**
 * Clear all events from the FIFO.
 * 
 * @param fifo Pointer to FIFO state
 */
void key_fifo_clear(key_fifo_t *fifo);

/**
 * Check if overflow occurred and clear the flag.
 * 
 * @param fifo Pointer to FIFO state
 * @return true if overflow occurred since last check
 */
bool key_fifo_check_and_clear_overflow(key_fifo_t *fifo);

/**
 * Encode an event entry.
 * 
 * @param event_type Event type
 * @param key_code Key code
 * @return Encoded event entry
 */
static inline uint8_t key_fifo_encode(uint8_t event_type, uint8_t key_code) {
    return ((key_code << KEY_FIFO_KEY_CODE_SHIFT) & KEY_FIFO_KEY_CODE_MASK) |
           ((event_type << KEY_FIFO_EVENT_TYPE_SHIFT) & KEY_FIFO_EVENT_TYPE_MASK);
}

/**
 * Decode event type from entry.
 * 
 * @param entry Encoded event entry
 * @return Event type
 */
static inline uint8_t key_fifo_decode_type(uint8_t entry) {
    return (entry & KEY_FIFO_EVENT_TYPE_MASK) >> KEY_FIFO_EVENT_TYPE_SHIFT;
}

/**
 * Decode key code from entry.
 * 
 * @param entry Encoded event entry
 * @return Key code
 */
static inline uint8_t key_fifo_decode_key_code(uint8_t entry) {
    return (entry & KEY_FIFO_KEY_CODE_MASK) >> KEY_FIFO_KEY_CODE_SHIFT;
}

#endif  // KEY_FIFO_H
