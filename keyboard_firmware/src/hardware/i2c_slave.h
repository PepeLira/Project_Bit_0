#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdbool.h>
#include <stdint.h>
#include "key_fifo.h"

// I2C slave configuration
#define I2C_SLAVE_SDA_GPIO 0
#define I2C_SLAVE_SCL_GPIO 1
#define I2C_SLAVE_DEFAULT_ADDRESS 0x20
#define I2C_SLAVE_BAUDRATE 100000  // 100 kHz

// Register addresses
#define I2C_REG_KEY_STATUS    0x00  // Key status: bits[3:0]=modifiers, bits[7:4]=FIFO level
#define I2C_REG_FIFO_ACCESS   0x01  // FIFO access: pop one event
#define I2C_REG_MOUSE_X       0x02  // Mouse X position/delta
#define I2C_REG_MOUSE_Y       0x03  // Mouse Y position/delta
#define I2C_REG_INTERRUPT     0x04  // Interrupt status: bit flags for interrupt sources

// Interrupt status register bit flags
#define I2C_INT_FIFO_OVERFLOW   (1 << 0)  // Bit 0: FIFO overflow occurred
#define I2C_INT_SHIFT_MOD       (1 << 1)  // Bit 1: SHIFT modifier changed
#define I2C_INT_FN_MOD          (1 << 2)  // Bit 2: FN modifier changed
#define I2C_INT_ALT_MOD         (1 << 3)  // Bit 3: ALT modifier changed
#define I2C_INT_KEY_EVENT       (1 << 4)  // Bit 4: Keyboard key pressed/released
#define I2C_INT_MOUSE_EVENT     (1 << 5)  // Bit 5: Mouse movement event
#define I2C_INT_POWER_BUTTON    (1 << 6)  // Bit 6: Power button event
// Bit 7 reserved

/**
 * Initialize the I2C slave interface.
 * 
 * @param address The I2C slave address (7-bit)
 * @param interrupt_gpio GPIO pin for interrupt output (or 0xFF for none)
 */
void i2c_slave_init(uint8_t address, uint8_t interrupt_gpio);

/**
 * Set the key FIFO that the I2C interface will read from.
 * 
 * @param fifo Pointer to the key FIFO
 */
void i2c_slave_set_fifo(key_fifo_t *fifo);

/**
 * Update the modifier state that will be reported via I2C.
 * 
 * @param modifier_mask Bitmask of active modifiers (bits [3:0])
 */
void i2c_slave_update_modifiers(uint8_t modifier_mask);

/**
 * Update the mouse position that will be reported via I2C.
 * 
 * @param x_delta X position delta (signed 8-bit)
 * @param y_delta Y position delta (signed 8-bit)
 */
void i2c_slave_update_mouse(int8_t x_delta, int8_t y_delta);

/**
 * Notify that new events are available in the FIFO.
 * This will assert the interrupt line if configured.
 */
void i2c_slave_notify_events_available(void);

/**
 * Check if the FIFO is empty and clear interrupt if needed.
 * Should be called after events are consumed.
 */
void i2c_slave_check_and_clear_interrupt(void);

/**
 * Set one or more interrupt status bits.
 * 
 * @param flags Bit flags to set (OR combination of I2C_INT_* defines)
 */
void i2c_slave_set_interrupt_flags(uint8_t flags);

/**
 * Clear one or more interrupt status bits.
 * Reading the interrupt register clears it automatically.
 * 
 * @param flags Bit flags to clear (OR combination of I2C_INT_* defines)
 */
void i2c_slave_clear_interrupt_flags(uint8_t flags);

/**
 * Get current interrupt status flags.
 * 
 * @return Current interrupt status byte
 */
uint8_t i2c_slave_get_interrupt_flags(void);

#endif  // I2C_SLAVE_H
