=====================================
Luckfox Lyra I2C Keyboard Driver
=====================================

:Author: Luckfox
:Date: 2025

Description
===========

This driver supports the Luckfox Lyra custom I2C keyboard and mouse device.
The device features:

- 53 programmable keys with 3-layer mapping (normal, shift, FN)
- Integrated mouse with relative movement
- Power button support
- FIFO-based event queue
- Hardware-handled debouncing and auto-repeat

The device is polled periodically (default 10ms) for events since it has no
interrupt output pin.

Device Tree Binding
===================

Required properties:

- compatible: Must be "luckfox,lyra-keyboard"
- reg: I2C slave address (typically 0x20)

Example::

    &i2c0 {
        status = "okay";
        clock-frequency = <100000>;

        lyra_keyboard: keyboard@20 {
            compatible = "luckfox,lyra-keyboard";
            reg = <0x20>;
            status = "okay";
        };
    };

Register Map
============

The device exposes 5 registers via I2C:

+----------+---------------+--------+------------------------------------------+
| Address  | Name          | Access | Description                              |
+==========+===============+========+==========================================+
| 0x00     | Key Status    | R      | Bits[3:0]: Modifiers (FN/ALT/SHIFT)      |
|          |               |        | Bits[7:4]: FIFO level (0-15)             |
+----------+---------------+--------+------------------------------------------+
| 0x01     | FIFO Access   | R      | Pop key event from FIFO                  |
|          |               |        | Bits[1:0]: Event type (press/hold/rel)   |
|          |               |        | Bits[7:2]: Key code (0-52)               |
|          |               |        | Returns 0x00 if empty                    |
+----------+---------------+--------+------------------------------------------+
| 0x02     | Mouse X       | R      | Signed 8-bit X delta (reading clears)    |
+----------+---------------+--------+------------------------------------------+
| 0x03     | Mouse Y       | R      | Signed 8-bit Y delta (reading clears)    |
+----------+---------------+--------+------------------------------------------+
| 0x04     | Int Status    | R      | Interrupt flags (read clears):           |
|          |               |        | Bit 0: FIFO overflow                     |
|          |               |        | Bit 1: SHIFT changed                     |
|          |               |        | Bit 2: FN changed                        |
|          |               |        | Bit 3: ALT changed                       |
|          |               |        | Bit 4: Key event                         |
|          |               |        | Bit 5: Mouse event                       |
|          |               |        | Bit 6: Power button changed              |
+----------+---------------+--------+------------------------------------------+

Input Devices
=============

The driver creates two input devices:

1. **Keyboard** (/dev/input/eventX)
   
   - Reports keyboard events (EV_KEY)
   - Supports auto-repeat (EV_REP)
   - Reports scan codes (MSC_SCAN)
   - Includes power button (KEY_POWER)

2. **Mouse** (/dev/input/eventY)
   
   - Reports relative movement (REL_X, REL_Y)
   - Reports mouse buttons (BTN_LEFT, BTN_RIGHT, BTN_MIDDLE)
   - Supports mouse wheel (REL_WHEEL)

Keymap
======

The driver uses a static 3-layer keymap:

- **Normal layer**: Standard key presses
- **Shift layer**: Activated when SHIFT is held
- **FN layer**: Activated when FN key is held

Total of 53 keys are supported (codes 0-52):

- Matrix keys: 0-41 (7 columns × 6 rows)
- Function keys FN1-FN6, FN8: 42-48
- Mouse control keys FN9-FN12: 49-52

See the driver source code for the complete keymap translation tables.

Sysfs Attributes
================

The driver exposes the following sysfs attributes under
``/sys/bus/i2c/devices/<i2c-bus>-00<addr>/``:

mouse_speed_x
-------------
:Type: Read/Write
:Range: 10-500
:Default: 100
:Description: Mouse X-axis speed multiplier as percentage.
              100 = 1x (normal), 200 = 2x (double speed)

Example::

    # Read current speed
    cat /sys/bus/i2c/devices/0-0020/mouse_speed_x
    
    # Set 1.5x speed
    echo 150 > /sys/bus/i2c/devices/0-0020/mouse_speed_x

mouse_speed_y
-------------
:Type: Read/Write
:Range: 10-500
:Default: 100
:Description: Mouse Y-axis speed multiplier as percentage.

poll_interval
-------------
:Type: Read/Write
:Range: 5-100 (milliseconds)
:Default: 10
:Description: Polling interval for checking device events.
              Lower values = lower latency but higher CPU usage.

Example::

    # Read current interval
    cat /sys/bus/i2c/devices/0-0020/poll_interval
    
    # Set 20ms polling (lower CPU usage)
    echo 20 > /sys/bus/i2c/devices/0-0020/poll_interval

Usage Examples
==============

Testing with evtest
--------------------

To test keyboard events::

    # Find the keyboard event device
    evtest
    # Select the "Luckfox Lyra Keyboard" device
    
    # Or directly:
    evtest /dev/input/by-id/*Lyra*Keyboard*

To test mouse events::

    evtest /dev/input/by-id/*Lyra*Mouse*

Monitoring events with input-events
------------------------------------

::

    # Install input-utils if needed
    apt-get install input-utils
    
    # List all input devices
    lsinput
    
    # Monitor keyboard
    input-events <event-number>

Checking I2C communication
---------------------------

::

    # Detect device on I2C bus 0
    i2cdetect -y 0
    
    # Should show device at address 0x20
    # Read registers manually
    i2cget -y 0 0x20 0x00  # Read key status
    i2cget -y 0 0x20 0x04  # Read interrupt status

Troubleshooting
===============

Driver not loading
------------------

1. Check device tree is correct::

    dtc -I fs /sys/firmware/devicetree/base | grep -A5 lyra

2. Verify I2C device is detected::

    i2cdetect -y 0

3. Check kernel log::

    dmesg | grep -i lyra

No events received
------------------

1. Verify input devices are created::

    ls -l /dev/input/by-id/*Lyra*

2. Check polling is active::

    cat /sys/bus/i2c/devices/0-0020/poll_interval

3. Monitor I2C traffic::

    i2cdump -y 0 0x20

FIFO overflow warnings
----------------------

If you see "FIFO overflow" in kernel log:

1. Reduce polling interval::

    echo 5 > /sys/bus/i2c/devices/0-0020/poll_interval

2. Check for I2C bus errors::

    dmesg | grep i2c

Module Parameters
=================

None. Configuration is done via sysfs attributes.

Power Management
================

The driver supports suspend/resume:

- On suspend: Polling work is cancelled
- On resume: Polling work is restarted
- No explicit power management to the device (always powered)

License
=======

This driver is licensed under GPL v2.

See COPYING for details.
