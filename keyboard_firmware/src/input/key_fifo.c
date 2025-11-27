#include "key_fifo.h"
#include <string.h>

void key_fifo_init(key_fifo_t *fifo) {
    memset(fifo->buffer, 0, sizeof(fifo->buffer));
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
    fifo->overflow = false;
}

bool key_fifo_push(key_fifo_t *fifo, uint8_t event_type, uint8_t key_code) {
    if (fifo->count >= KEY_FIFO_SIZE) {
        fifo->overflow = true;
        return false;  // FIFO full
    }
    
    // Encode and store the event
    fifo->buffer[fifo->tail] = key_fifo_encode(event_type, key_code);
    fifo->tail = (fifo->tail + 1) % KEY_FIFO_SIZE;
    fifo->count++;
    
    return true;
}

uint8_t key_fifo_pop(key_fifo_t *fifo) {
    if (fifo->count == 0) {
        return KEY_FIFO_NO_EVENT;  // FIFO empty
    }
    
    uint8_t entry = fifo->buffer[fifo->head];
    fifo->head = (fifo->head + 1) % KEY_FIFO_SIZE;
    fifo->count--;
    
    return entry;
}

uint8_t key_fifo_peek(const key_fifo_t *fifo) {
    if (fifo->count == 0) {
        return KEY_FIFO_NO_EVENT;
    }
    
    return fifo->buffer[fifo->head];
}

uint8_t key_fifo_count(const key_fifo_t *fifo) {
    return fifo->count;
}

bool key_fifo_is_empty(const key_fifo_t *fifo) {
    return fifo->count == 0;
}

bool key_fifo_is_full(const key_fifo_t *fifo) {
    return fifo->count >= KEY_FIFO_SIZE;
}

void key_fifo_clear(key_fifo_t *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
}

bool key_fifo_check_and_clear_overflow(key_fifo_t *fifo) {
    bool had_overflow = fifo->overflow;
    fifo->overflow = false;
    return had_overflow;
}
