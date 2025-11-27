# Interrupt Status Register Implementation

## Overview
A new interrupt status register (0x04) has been added to the I2C slave interface to provide detailed interrupt source tracking. This allows the I2C master to determine exactly what caused an interrupt event.

## Register Definition

**Address:** 0x04  
**Name:** Interrupt Status  
**Access:** Read (read-clears all bits)

### Bit Mapping

| Bit | Name | Description |
|-----|------|-------------|
| 0 | FIFO_OVERFLOW | FIFO overflow occurred (event was dropped) |
| 1 | SHIFT_MOD | SHIFT modifier state changed |
| 2 | FN_MOD | FN modifier state changed |
| 3 | ALT_MOD | ALT modifier state changed |
| 4 | KEY_EVENT | Keyboard key pressed/released (matrix or FN keys) |
| 5 | MOUSE_EVENT | Mouse movement event occurred |
| 6 | POWER_BUTTON | Power button state changed |
| 7 | RESERVED | Reserved (always 0) |

## Implementation Details

### Files Modified

1. **src/hardware/i2c_slave.h**
   - Added `I2C_REG_INTERRUPT` register definition (0x04)
   - Added interrupt bit flag constants (I2C_INT_*)
   - Added functions: `i2c_slave_set_interrupt_flags()`, `i2c_slave_clear_interrupt_flags()`, `i2c_slave_get_interrupt_flags()`

2. **src/hardware/i2c_slave.c**
   - Added `interrupt_status` volatile variable
   - Implemented interrupt flag manipulation functions
   - Added register 0x04 case to I2C read handler (read-clears all flags)
   - Modified interrupt GPIO handling to use interrupt status flags

3. **src/input/key_fifo.h/.c**
   - Added `overflow` flag to `key_fifo_t` structure
   - Implemented `key_fifo_check_and_clear_overflow()` function
   - Set overflow flag when FIFO push fails

4. **src/app/main.c**
   - Added tracking for previous power button and modifier states
   - Set `I2C_INT_POWER_BUTTON` when power button state changes
   - Set `I2C_INT_KEY_EVENT` when matrix or FN keyboard events occur
   - Set `I2C_INT_FN_MOD`, `I2C_INT_ALT_MOD`, `I2C_INT_SHIFT_MOD` when modifiers change
   - Set `I2C_INT_MOUSE_EVENT` when mouse movement occurs
   - Set `I2C_INT_FIFO_OVERFLOW` when FIFO overflow is detected

5. **README.md**
   - Added register 0x04 to register map table
   - Added detailed usage examples for interrupt status register
   - Updated interrupt signaling section to reference new register

## Interrupt Flag Behavior

### Setting Flags
- Flags are set automatically by firmware when events occur
- Multiple flags can be set simultaneously
- Setting any flag asserts the interrupt GPIO line (active-low)

### Clearing Flags
- Reading register 0x04 automatically clears ALL interrupt flags
- Clearing all flags de-asserts the interrupt GPIO line (returns to high)

### Typical Usage Pattern

1. Master detects interrupt line going LOW (GPIO 26)
2. Master reads register 0x04 to identify interrupt sources
3. Master processes appropriate registers based on flags:
   - If FIFO_OVERFLOW → handle lost events
   - If SHIFT_MOD/FN_MOD/ALT_MOD → read 0x00 for current modifier state
   - If KEY_EVENT → read 0x01 to pop FIFO events
   - If MOUSE_EVENT → read 0x02 and 0x03 for mouse deltas
   - If POWER_BUTTON → handle power state change
4. Reading 0x04 clears all flags and de-asserts interrupt line

## Example Scenarios

### Scenario 1: User presses a key with FN modifier active
```
Firmware sets:
- I2C_INT_FN_MOD (0x04) - FN modifier activated
- I2C_INT_KEY_EVENT (0x10) - Key was pressed

Register 0x04 = 0x14

Master reads:
W: 0x20 0x04
R: 0x21 0x14

Master interprets: FN modifier and key event occurred
```

### Scenario 2: FIFO overflow during rapid typing
```
Firmware sets:
- I2C_INT_FIFO_OVERFLOW (0x01) - Event dropped
- I2C_INT_KEY_EVENT (0x10) - Keys were pressed

Register 0x04 = 0x11

Master reads:
W: 0x20 0x04
R: 0x21 0x11

Master interprets: Events were lost, increase polling rate
```

### Scenario 3: Mouse movement only
```
Firmware sets:
- I2C_INT_MOUSE_EVENT (0x20) - Mouse moved

Register 0x04 = 0x20

Master reads:
W: 0x20 0x04
R: 0x21 0x20

Master interprets: Mouse movement, read 0x02/0x03 for deltas
```

## Benefits

1. **Precise Interrupt Identification:** Master knows exactly what caused the interrupt
2. **Reduced I2C Traffic:** Master only reads relevant registers
3. **Overflow Detection:** System can detect and handle FIFO overflow conditions
4. **Event Prioritization:** Master can prioritize handling based on interrupt type
5. **State Change Tracking:** Modifier and power button changes are explicitly flagged

## Build Status

✅ **Implementation Complete**
✅ **Firmware builds successfully**
✅ **Ready for testing**

Build artifacts: `build/i2c_keyboard.uf2` (30KB)
