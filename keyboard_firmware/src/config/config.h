#ifndef CONFIG_H
#define CONFIG_H

// GPIO assignments
#define CONFIG_POWER_LATCH_GPIO 29
#define CONFIG_LED_GPIO 28

// I2C configuration
#define CONFIG_I2C_SDA_GPIO 0
#define CONFIG_I2C_SCL_GPIO 1
#define CONFIG_I2C_SLAVE_ADDRESS 0x20
#define CONFIG_I2C_INTERRUPT_GPIO 26  // Interrupt output for event signaling

// Matrix keyboard rows (6 rows)
#define CONFIG_ROW_1_GPIO 7
#define CONFIG_ROW_2_GPIO 8
#define CONFIG_ROW_3_GPIO 9
#define CONFIG_ROW_4_GPIO 10
#define CONFIG_ROW_5_GPIO 11
#define CONFIG_ROW_6_GPIO 2

// Matrix keyboard columns (7 columns)
#define CONFIG_COL_A_GPIO 12
#define CONFIG_COL_B_GPIO 13
#define CONFIG_COL_C_GPIO 14
#define CONFIG_COL_D_GPIO 15
#define CONFIG_COL_E_GPIO 16
#define CONFIG_COL_F_GPIO 17
#define CONFIG_COL_G_GPIO 18

// Independent FN keys (11 keys, FN7 is skipped)
#define CONFIG_FN1_GPIO 19
#define CONFIG_FN2_GPIO 20
#define CONFIG_FN3_GPIO 21
#define CONFIG_FN4_GPIO 22
#define CONFIG_FN5_GPIO 3
#define CONFIG_FN6_GPIO 4
#define CONFIG_FN8_GPIO 5
#define CONFIG_FN9_GPIO 6
#define CONFIG_FN10_GPIO 23
#define CONFIG_FN11_GPIO 24
#define CONFIG_FN12_GPIO 25

// Modifier key positions in matrix (from keyboard_layout.json)
// C6 = FN (col 2, row 5)
// C5 = ALT (col 2, row 4)
// E4 = LSHIFT (col 4, row 3)
#define MODIFIER_FN_ROW 5
#define MODIFIER_FN_COL 2
#define MODIFIER_ALT_ROW 4
#define MODIFIER_ALT_COL 2
#define MODIFIER_SHIFT_ROW 3
#define MODIFIER_SHIFT_COL 4

// LED colors encoded as 0xRRGGBB
#define CONFIG_COLOR_IDLE 0x001400        // Green - idle/running
#define CONFIG_COLOR_POWER 0x140000       // Red - power button pressed
#define CONFIG_COLOR_PULSE 0x200400       // Orange - short press pulse
#define CONFIG_COLOR_MOD_FN 0x200C00      // Orange - FN modifier active
#define CONFIG_COLOR_MOD_ALT 0x0C2000     // Yellow-Green - ALT modifier active
#define CONFIG_COLOR_MOD_SHIFT 0x00200C   // Cyan - SHIFT modifier active

// Timers
#define DEBOUNCE_MS 30
#define STARTUP_WINDOW_MS 1000
#define FIRST_PRESS_HOLD_MS 500
#define LONG_PRESS_MS 3000
#define MODIFIER_DOUBLE_PRESS_WINDOW_MS 300
#define MOUSE_UPDATE_INTERVAL_MS 20

#endif  // CONFIG_H
