# Luckfox Lyra I2C Keyboard Driver - Quick Start Guide

## Overview

This driver implements support for the Luckfox Lyra custom I2C keyboard with:
- 53 keys with 3-layer mapping (normal/shift/FN)
- Integrated mouse with configurable speed
- Power button support
- Polling-based operation (no IRQ required)

## Building

### 1. Enable the driver in kernel config

```bash
cd /home/pepe/Lyra-sdk/kernel-6.1

# Using menuconfig
make ARCH=arm menuconfig
# Navigate to: Device Drivers -> Input device support -> Keyboards
# Enable: <M> Luckfox Lyra I2C Keyboard and Mouse

# Or edit .config directly
echo "CONFIG_KEYBOARD_LYRA_I2C=m" >> .config
```

### 2. Build kernel and modules

```bash
# From kernel-6.1 directory
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc)
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- modules
```

### 3. Build device tree

```bash
make ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- dtbs
```

## Installation

### 1. Copy kernel modules

```bash
# Copy to target device (adjust path as needed)
scp drivers/input/keyboard/lyra_i2c_keyboard.ko root@device:/lib/modules/$(uname -r)/kernel/drivers/input/keyboard/

# On device, update module dependencies
depmod -a
```

### 2. Copy device tree blob

```bash
# Copy updated DTB
scp arch/arm/boot/dts/rk3506-luckfox-lyra.dtb root@device:/boot/

# On device, backup old DTB and use new one
# (method depends on your bootloader configuration)
```

### 3. Reboot device

```bash
reboot
```

## Testing

### 1. Check driver loaded

```bash
# Check kernel log
dmesg | grep -i lyra

# Expected output:
# [   X.XXXXXX] lyra-i2c-keyboard 0-0020: Luckfox Lyra keyboard/mouse initialized

# Check module
lsmod | grep lyra

# Check I2C device
i2cdetect -y 0
# Should show device at address 0x20
```

### 2. Verify input devices

```bash
# List input devices
ls -l /dev/input/by-id/

# Should show:
# usb-*Luckfox_Lyra_Keyboard*
# usb-*Luckfox_Lyra_Mouse*

# Or check /proc/bus/input/devices
cat /proc/bus/input/devices | grep -A10 "Lyra"
```

### 3. Test keyboard events

```bash
# Install evtest if needed
apt-get install evtest

# Test keyboard
evtest /dev/input/by-id/*Lyra*Keyboard*

# Press keys and verify events appear
```

### 4. Test mouse

```bash
# Test mouse
evtest /dev/input/by-id/*Lyra*Mouse*

# Move the mouse (FN9-FN12 keys) and verify REL_X/REL_Y events
```

### 5. Adjust mouse speed

```bash
# Check current speed (default 100 = 1x)
cat /sys/bus/i2c/devices/0-0020/mouse_speed_x
cat /sys/bus/i2c/devices/0-0020/mouse_speed_y

# Set faster speed (200 = 2x)
echo 200 > /sys/bus/i2c/devices/0-0020/mouse_speed_x
echo 200 > /sys/bus/i2c/devices/0-0020/mouse_speed_y

# Set slower speed (50 = 0.5x)
echo 50 > /sys/bus/i2c/devices/0-0020/mouse_speed_x
```

### 6. Adjust polling interval

```bash
# Check current interval (default 10ms)
cat /sys/bus/i2c/devices/0-0020/poll_interval

# Reduce latency (5ms)
echo 5 > /sys/bus/i2c/devices/0-0020/poll_interval

# Reduce CPU usage (20ms)
echo 20 > /sys/bus/i2c/devices/0-0020/poll_interval
```

## Troubleshooting

### Driver not loading

1. **Check I2C bus:**
   ```bash
   i2cdetect -y 0
   # Device should appear at 0x20
   ```

2. **Check device tree:**
   ```bash
   dtc -I fs /sys/firmware/devicetree/base | grep -A10 lyra
   ```

3. **Manual module load:**
   ```bash
   modprobe lyra_i2c_keyboard
   dmesg | tail -20
   ```

### No events received

1. **Check register values:**
   ```bash
   # Read key status
   i2cget -y 0 0x20 0x00
   
   # Read interrupt status
   i2cget -y 0 0x20 0x04
   
   # Read FIFO
   i2cget -y 0 0x20 0x01
   ```

2. **Check polling is active:**
   ```bash
   cat /sys/bus/i2c/devices/0-0020/poll_interval
   # Should show a value (e.g., 10)
   ```

3. **Monitor kernel log:**
   ```bash
   dmesg -w | grep -i lyra
   ```

### FIFO overflow warnings

If kernel log shows "FIFO overflow detected":

```bash
# Reduce polling interval
echo 5 > /sys/bus/i2c/devices/0-0020/poll_interval
```

### I2C communication errors

```bash
# Check I2C bus errors
dmesg | grep i2c

# Try different I2C speed (in device tree)
# clock-frequency = <100000>;  // Default 100kHz
# clock-frequency = <400000>;  // Try 400kHz
```

## Key Mapping Reference

### Normal Layer (No Modifiers)
- Matrix keys 0-41: Standard QWERTY layout
- Key 37 (C6): FN modifier
- Key 25 (E4): Left Shift
- Key 41 (G6): Right Shift
- Key 30 (C5): Alt
- Key 33 (F5): Ctrl
- Keys 42-47 (FN1-FN6): w,a,s,x,j,k
- Key 48 (FN8): Mouse left click
- Keys 49-52 (FN9-FN12): Mouse movement keys

### FN Layer
- Keys 0-6: F4, F5, F7, F6, F8, F9, F10
- Key 12-13: F11, F12
- Key 18: END
- Key 19: HOME
- Key 21: F3
- Key 28: F2
- Key 35: F1
- Keys 42-45: Arrow keys (UP/LEFT/RIGHT/DOWN)

See `kernel-6.1/drivers/input/keyboard/lyra_i2c_keyboard.c` for complete keymap.

## Configuration Files

### Device Tree
- Location: `kernel-6.1/arch/arm/boot/dts/rk3506-luckfox-lyra.dtsi`
- Node: `&i2c0 -> lyra_keyboard@20`

### Kernel Config
- Option: `CONFIG_KEYBOARD_LYRA_I2C`
- Location: `kernel-6.1/drivers/input/keyboard/Kconfig`

### Driver Source
- Location: `kernel-6.1/drivers/input/keyboard/lyra_i2c_keyboard.c`

### Documentation
- Full docs: `kernel-6.1/Documentation/input/devices/lyra-i2c-keyboard.rst`

## Support

For issues or questions, check:
1. Kernel log: `dmesg | grep -i lyra`
2. I2C bus: `i2cdetect -y 0`
3. Input devices: `evtest`
4. Register dump: `i2cdump -y 0 0x20`
