#include "i2c_slave.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// Use I2C0 peripheral
#ifndef I2C_SLAVE_INSTANCE
#define I2C_SLAVE_INSTANCE i2c0
#endif

// I2C state
static key_fifo_t *fifo_ptr = NULL;
static uint8_t interrupt_gpio = 0xFF;
static uint8_t current_register = 0x00;

// Register data - volatile because accessed in IRQ context
static volatile uint8_t modifier_mask = 0;
static volatile int8_t mouse_x_delta = 0;
static volatile int8_t mouse_y_delta = 0;
static volatile uint8_t interrupt_status = 0;

// I2C slave IRQ handler
static void i2c_slave_irq_handler(void) {
    uint32_t status = i2c0->hw->intr_stat;
    
    // Check if master sent us data (RX_FULL) - register address write
    if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        // Read the register address
        current_register = (uint8_t)i2c0->hw->data_cmd;
    }
    
    // Check if master is reading from us (RD_REQ)
    if (status & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        uint8_t data = 0;
        
        // Serve data based on current register
        switch (current_register) {
            case I2C_REG_KEY_STATUS: {
                // Build status register
                uint8_t fifo_level = 0;
                if (fifo_ptr != NULL) {
                    fifo_level = key_fifo_count(fifo_ptr);
                    if (fifo_level > 15) {
                        fifo_level = 15;  // Max 4 bits
                    }
                }
                data = (fifo_level << 4) | (modifier_mask & 0x0F);
                break;
            }
            
            case I2C_REG_FIFO_ACCESS: {
                // Pop one event from FIFO
                if (fifo_ptr != NULL) {
                    data = key_fifo_pop(fifo_ptr);
                } else {
                    data = KEY_FIFO_NO_EVENT;
                }
                break;
            }
            
            case I2C_REG_MOUSE_X:
                data = (uint8_t)mouse_x_delta;
                break;
            
            case I2C_REG_MOUSE_Y:
                data = (uint8_t)mouse_y_delta;
                break;
            
            case I2C_REG_INTERRUPT:
                data = interrupt_status;
                // Reading interrupt register clears it
                interrupt_status = 0;
                break;
            
            default:
                data = 0x00;  // Reserved/invalid register
                break;
        }
        
        // Send the data
        i2c0->hw->data_cmd = data;
        
        // Clear the RD_REQ interrupt
        i2c0->hw->clr_rd_req;
    }
    
    // Clear STOP_DET interrupt if set
    if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        i2c0->hw->clr_stop_det;
        
        // Check if FIFO is now empty and clear interrupt
        if (fifo_ptr != NULL && key_fifo_is_empty(fifo_ptr)) {
            if (interrupt_gpio != 0xFF) {
                gpio_put(interrupt_gpio, 1);  // Deassert (active low)
            }
        }
    }
}

void i2c_slave_init(uint8_t address, uint8_t int_gpio) {
    interrupt_gpio = int_gpio;
    
    // Initialize interrupt GPIO if provided
    if (interrupt_gpio != 0xFF) {
        gpio_init(interrupt_gpio);
        gpio_set_dir(interrupt_gpio, GPIO_OUT);
        gpio_put(interrupt_gpio, 1);  // Deasserted (active low)
    }
    
    // Initialize I2C pins
    gpio_set_function(I2C_SLAVE_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLAVE_SCL_GPIO, GPIO_FUNC_I2C);
    
    // Enable pull-ups on I2C pins (required for I2C)
    gpio_pull_up(I2C_SLAVE_SDA_GPIO);
    gpio_pull_up(I2C_SLAVE_SCL_GPIO);
    
    // Initialize I2C peripheral at specified baudrate
    i2c_init(I2C_SLAVE_INSTANCE, I2C_SLAVE_BAUDRATE);
    
    // Disable I2C to configure it
    i2c0->hw->enable = 0;
    
    // Set slave address
    i2c0->hw->sar = address;
    
    // Configure as slave (clear MASTER_MODE and IC_SLAVE_DISABLE)
    i2c0->hw->con = I2C_IC_CON_IC_SLAVE_DISABLE_BITS | I2C_IC_CON_IC_RESTART_EN_BITS |
                    I2C_IC_CON_TX_EMPTY_CTRL_BITS;
    i2c0->hw->con &= ~(I2C_IC_CON_MASTER_MODE_BITS | I2C_IC_CON_IC_SLAVE_DISABLE_BITS);
    
    // Enable interrupts for slave operations
    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RD_REQ_BITS |
                          I2C_IC_INTR_MASK_M_RX_FULL_BITS |
                          I2C_IC_INTR_MASK_M_STOP_DET_BITS;
    
    // Enable I2C
    i2c0->hw->enable = 1;
    
    // Set up the IRQ handler
    irq_set_exclusive_handler(I2C0_IRQ, i2c_slave_irq_handler);
    irq_set_enabled(I2C0_IRQ, true);
    
    // Initialize register data
    modifier_mask = 0;
    mouse_x_delta = 0;
    mouse_y_delta = 0;
    interrupt_status = 0;
    current_register = 0x00;
    fifo_ptr = NULL;
}

void i2c_slave_set_fifo(key_fifo_t *fifo) {
    fifo_ptr = fifo;
}

void i2c_slave_update_modifiers(uint8_t mod_mask) {
    modifier_mask = mod_mask & 0x0F;  // Only 4 bits
}

void i2c_slave_update_mouse(int8_t x_delta, int8_t y_delta) {
    mouse_x_delta = x_delta;
    mouse_y_delta = y_delta;
}

void i2c_slave_notify_events_available(void) {
    if (interrupt_gpio != 0xFF && fifo_ptr != NULL && !key_fifo_is_empty(fifo_ptr)) {
        gpio_put(interrupt_gpio, 0);  // Assert (active low)
    }
}

void i2c_slave_check_and_clear_interrupt(void) {
    if (interrupt_gpio != 0xFF && fifo_ptr != NULL && key_fifo_is_empty(fifo_ptr)) {
        gpio_put(interrupt_gpio, 1);  // Deassert (active low)
    }
}

void i2c_slave_set_interrupt_flags(uint8_t flags) {
    interrupt_status |= flags;
    // Assert interrupt line if any flags are set
    if (interrupt_gpio != 0xFF && interrupt_status != 0) {
        gpio_put(interrupt_gpio, 0);  // Assert (active low)
    }
}

void i2c_slave_clear_interrupt_flags(uint8_t flags) {
    interrupt_status &= ~flags;
    // Deassert interrupt line if no flags are set
    if (interrupt_gpio != 0xFF && interrupt_status == 0) {
        gpio_put(interrupt_gpio, 1);  // Deassert (active low)
    }
}

uint8_t i2c_slave_get_interrupt_flags(void) {
    return interrupt_status;
}
