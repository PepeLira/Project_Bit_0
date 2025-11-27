// SPDX-License-Identifier: GPL-2.0-only
/*
 * Luckfox Lyra I2C Keyboard and Mouse Driver
 *
 * Copyright (C) 2025
 *
 * This driver supports a custom I2C keyboard/mouse device with:
 * - 53 keys with normal/shift/fn modifiers
 * - Relative mouse input with configurable speed
 * - Power button support
 * - FIFO-based event queue
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/of.h>

/* Register addresses */
#define REG_KEY_STATUS		0x00
#define REG_FIFO_ACCESS		0x01
#define REG_MOUSE_X		0x02
#define REG_MOUSE_Y		0x03
#define REG_INT_STATUS		0x04

/* Register bit definitions */
#define KEY_STATUS_SHIFT_BIT	BIT(0)
#define KEY_STATUS_ALT_BIT	BIT(1)
#define KEY_STATUS_FN_BIT	BIT(2)
#define KEY_STATUS_FIFO_MASK	0xF0
#define KEY_STATUS_FIFO_SHIFT	4

#define FIFO_EVENT_TYPE_MASK	0x03
#define FIFO_EVENT_NONE		0x00
#define FIFO_EVENT_PRESS	0x01
#define FIFO_EVENT_HOLD		0x02
#define FIFO_EVENT_RELEASE	0x03
#define FIFO_KEYCODE_MASK	0xFC
#define FIFO_KEYCODE_SHIFT	2

#define INT_STATUS_FIFO_OVERFLOW	BIT(0)
#define INT_STATUS_SHIFT_CHANGE		BIT(1)
#define INT_STATUS_FN_CHANGE		BIT(2)
#define INT_STATUS_ALT_CHANGE		BIT(3)
#define INT_STATUS_KEY_EVENT		BIT(4)
#define INT_STATUS_MOUSE_EVENT		BIT(5)
#define INT_STATUS_POWER_BTN		BIT(6)

#define MAX_KEYCODES		53
#define POLL_INTERVAL_MS	10
#define FIFO_MAX_READ		16

struct lyra_kbd_data {
	struct i2c_client *client;
	struct input_dev *kbd_input;
	struct input_dev *mouse_input;
	struct delayed_work poll_work;
	
	/* Track last pressed key for each keycode to ensure proper release */
	unsigned short last_key_pressed[MAX_KEYCODES];
	
	/* Mouse speed multiplier (percentage: 100 = 1x, 200 = 2x) */
	int mouse_speed_x;
	int mouse_speed_y;
	
	/* Power button state */
	bool power_btn_pressed;
	
	/* Polling interval */
	unsigned int poll_interval_ms;
};

/* Static keymap based on keyboard_layout.json */
/* Index = keycode (0-52), value = Linux key code */

/* Normal layer (no modifiers) */
static const unsigned short keymap_normal[MAX_KEYCODES] = {
	KEY_4,		/* 0: A1 */
	KEY_5,		/* 1: B1 */
	KEY_7,		/* 2: C1 */
	KEY_6,		/* 3: D1 */
	KEY_8,		/* 4: E1 */
	KEY_9,		/* 5: F1 */
	KEY_0,		/* 6: G1 */
	KEY_R,		/* 7: A2 */
	KEY_T,		/* 8: B2 */
	KEY_U,		/* 9: C2 */
	KEY_Y,		/* 10: D2 */
	KEY_I,		/* 11: E2 */
	KEY_O,		/* 12: F2 */
	KEY_P,		/* 13: G2 */
	KEY_F,		/* 14: A3 */
	KEY_G,		/* 15: B3 */
	KEY_COMMA,	/* 16: C3 */
	KEY_H,		/* 17: D3 */
	KEY_DOT,	/* 18: E3 */
	KEY_L,		/* 19: F3 */
	KEY_ENTER,	/* 20: G3 */
	KEY_3,		/* 21: A4 */
	KEY_E,		/* 22: B4 */
	KEY_C,		/* 23: C4 */
	KEY_D,		/* 24: D4 */
	KEY_LEFTSHIFT,	/* 25: E4 */
	KEY_M,		/* 26: F4 */
	KEY_SPACE,	/* 27: G4 - SPACEBAR */
	KEY_2,		/* 28: A5 */
	KEY_ESC,	/* 29: B5 */
	KEY_LEFTALT,	/* 30: C5 */
	KEY_TAB,	/* 31: D5 */
	KEY_V,		/* 32: E5 */
	KEY_LEFTCTRL,	/* 33: F5 */
	KEY_BACKSPACE,	/* 34: G5 */
	KEY_1,		/* 35: A6 */
	KEY_Q,		/* 36: B6 */
	KEY_FN,		/* 37: C6 */
	KEY_Z,		/* 38: D6 */
	KEY_B,		/* 39: E6 */
	KEY_N,		/* 40: F6 */
	KEY_RIGHTSHIFT,	/* 41: G6 */
	KEY_W,		/* 42: FN1 */
	KEY_A,		/* 43: FN2 */
	KEY_S,		/* 44: FN3 */
	KEY_X,		/* 45: FN4 */
	KEY_J,		/* 46: FN5 */
	KEY_K,		/* 47: FN6 */
	BTN_LEFT,	/* 48: FN8 - mouse left click */
	KEY_DOWN,	/* 49: FN9 - used for mouse down when not in mouse mode */
	KEY_UP,		/* 50: FN10 - used for mouse up when not in mouse mode */
	KEY_RIGHT,	/* 51: FN11 - used for mouse right when not in mouse mode */
	KEY_LEFT,	/* 52: FN12 - used for mouse left when not in mouse mode */
};

/* Shift layer */
static const unsigned short keymap_shift[MAX_KEYCODES] = {
	KEY_4,		/* 0: $ (shift+4) */
	KEY_5,		/* 1: % (shift+5) */
	KEY_7,		/* 2: & (shift+7) */
	KEY_6,		/* 3: ^ (shift+6) */
	KEY_8,		/* 4: * (shift+8) */
	KEY_9,		/* 5: ( (shift+9) */
	KEY_0,		/* 6: ) (shift+0) */
	KEY_R,		/* 7: R */
	KEY_T,		/* 8: T */
	KEY_U,		/* 9: U */
	KEY_Y,		/* 10: Y */
	KEY_I,		/* 11: I */
	KEY_O,		/* 12: O */
	KEY_P,		/* 13: P */
	KEY_F,		/* 14: F */
	KEY_G,		/* 15: G */
	KEY_COMMA,	/* 16: < (shift+comma) */
	KEY_H,		/* 17: H */
	KEY_DOT,	/* 18: > (shift+dot) */
	KEY_L,		/* 19: L */
	KEY_ENTER,	/* 20: ENTER */
	KEY_3,		/* 21: # (shift+3) */
	KEY_E,		/* 22: E */
	KEY_C,		/* 23: C */
	KEY_D,		/* 24: D */
	KEY_LEFTSHIFT,	/* 25: LSHIFT */
	KEY_M,		/* 26: M */
	KEY_SPACE,	/* 27: SPACEBAR */
	KEY_2,		/* 28: @ (shift+2) */
	KEY_ESC,	/* 29: ESC */
	KEY_LEFTALT,	/* 30: ALT */
	KEY_TAB,	/* 31: TAB */
	KEY_V,		/* 32: V */
	KEY_LEFTCTRL,	/* 33: CTRL */
	KEY_BACKSPACE,	/* 34: BACKSPACE */
	KEY_1,		/* 35: ! (shift+1) */
	KEY_Q,		/* 36: Q */
	KEY_FN,		/* 37: FN */
	KEY_Z,		/* 38: Z */
	KEY_B,		/* 39: B */
	KEY_N,		/* 40: N */
	KEY_RIGHTSHIFT,	/* 41: RSHIFT */
	KEY_W,		/* 42: W */
	KEY_A,		/* 43: A */
	KEY_S,		/* 44: S */
	KEY_X,		/* 45: X */
	KEY_J,		/* 46: J */
	KEY_K,		/* 47: K */
	BTN_RIGHT,	/* 48: FN8 - mouse right click */
	KEY_DOWN,	/* 49: FN9 */
	KEY_UP,		/* 50: FN10 */
	KEY_RIGHT,	/* 51: FN11 */
	KEY_LEFT,	/* 52: FN12 */
};

/* FN layer */
static const unsigned short keymap_fn[MAX_KEYCODES] = {
	KEY_F4,		/* 0: A1 */
	KEY_F5,		/* 1: B1 */
	KEY_F7,		/* 2: C1 */
	KEY_F6,		/* 3: D1 */
	KEY_F8,		/* 4: E1 */
	KEY_F9,		/* 5: F1 */
	KEY_F10,	/* 6: G1 */
	KEY_MINUS,	/* 7: _ */
	KEY_MINUS,	/* 8: - */
	KEY_EQUAL,	/* 9: + */
	KEY_EQUAL,	/* 10: = */
	KEY_BACKSLASH,	/* 11: \ */
	KEY_F11,	/* 12: F11 */
	KEY_F12,	/* 13: F12 */
	KEY_APOSTROPHE,	/* 14: " */
	KEY_LEFTBRACE,	/* 15: { */
	KEY_SLASH,	/* 16: / */
	KEY_RIGHTBRACE,	/* 17: } */
	KEY_END,	/* 18: END */
	KEY_HOME,	/* 19: HOME */
	KEY_ENTER,	/* 20: ENTER */
	KEY_F3,		/* 21: F3 */
	KEY_GRAVE,	/* 22: ` */
	KEY_SEMICOLON,	/* 23: ; */
	KEY_SEMICOLON,	/* 24: : (shift+;) */
	KEY_LEFTSHIFT,	/* 25: LSHIFT */
	KEY_SLASH,	/* 26: ? (shift+/) */
	KEY_SPACE,	/* 27: SPACEBAR */
	KEY_F2,		/* 28: F2 */
	KEY_ESC,	/* 29: ESC */
	KEY_LEFTALT,	/* 30: ALT */
	KEY_TAB,	/* 31: TAB */
	KEY_APOSTROPHE,	/* 32: ' */
	KEY_LEFTCTRL,	/* 33: CTRL */
	KEY_BACKSPACE,	/* 34: BACKSPACE */
	KEY_F1,		/* 35: F1 */
	KEY_GRAVE,	/* 36: ~ (shift+`) */
	KEY_FN,		/* 37: FN */
	KEY_102ND,	/* 38: | */
	KEY_LEFTBRACE,	/* 39: [ */
	KEY_RIGHTBRACE,	/* 40: ] */
	KEY_RIGHTSHIFT,	/* 41: RSHIFT */
	KEY_UP,		/* 42: FN1+FN = UP */
	KEY_LEFT,	/* 43: FN2+FN = LEFT */
	KEY_RIGHT,	/* 44: FN3+FN = RIGHT */
	KEY_DOWN,	/* 45: FN4+FN = DOWN */
	KEY_A,		/* 46: FN5+FN = A (gamepad) */
	KEY_B,		/* 47: FN6+FN = B (gamepad) */
	BTN_MIDDLE,	/* 48: FN8+FN = middle click */
	KEY_DOWN,	/* 49: FN9+FN = DOWN */
	KEY_UP,		/* 50: FN10+FN = UP */
	KEY_RIGHT,	/* 51: FN11+FN = RIGHT */
	KEY_LEFT,	/* 52: FN12+FN = LEFT */
};

static int lyra_kbd_read_reg(struct i2c_client *client, u8 reg)
{
	int ret;
	
	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "Failed to read reg 0x%02x: %d\n", reg, ret);
	
	return ret;
}

static void lyra_kbd_process_key_event(struct lyra_kbd_data *kbd, u8 keycode, 
					bool pressed)
{
	unsigned short key;
	int ret;
	u8 key_status;
	bool shift, alt, fn;
	
	if (keycode >= MAX_KEYCODES) {
		dev_warn(&kbd->client->dev, "Invalid keycode: %d\n", keycode);
		return;
	}
	
	/* Read current modifier state from hardware */
	ret = lyra_kbd_read_reg(kbd->client, REG_KEY_STATUS);
	if (ret < 0)
		return;
	
	key_status = (u8)ret;
	shift = (key_status & KEY_STATUS_SHIFT_BIT) != 0;
	alt = (key_status & KEY_STATUS_ALT_BIT) != 0;
	fn = (key_status & KEY_STATUS_FN_BIT) != 0;
	
	/* Debug logging */
	dev_info(&kbd->client->dev, "Key event: code=%d pressed=%d shift=%d alt=%d fn=%d\n",
		 keycode, pressed, shift, alt, fn);
	
	/* Handle modifier keys - report them directly */
	if (keycode == 25 || keycode == 41) { /* LSHIFT/RSHIFT */
		/* Shift is handled via REG_KEY_STATUS in lyra_kbd_sync_modifiers */
		return;
	}
	
	if (keycode == 30) { /* ALT key */
		/* Alt is handled via REG_KEY_STATUS in lyra_kbd_sync_modifiers */
		return;
	}
	
	if (keycode == 33) { /* CTRL key */
		input_report_key(kbd->kbd_input, KEY_LEFTCTRL, pressed);
		input_sync(kbd->kbd_input);
		return;
	}
	
	if (keycode == 37) { /* FN key - modifier only, don't report */
		return;
	}
	
	if (pressed) {
		/* Select keymap based on modifiers */
		if (fn)
			key = keymap_fn[keycode];
		else if (shift)
			key = keymap_shift[keycode];
		else
			key = keymap_normal[keycode];
		
		/* Debug: what key are we reporting? */
		dev_info(&kbd->client->dev, "  -> Reporting linux_keycode=%d\n", key);
		
		/* Store which key we pressed so we can release the same one */
		kbd->last_key_pressed[keycode] = key;
		dev_info(&kbd->client->dev, "  -> input_report_key(dev, %d, 1)\n", key);
		input_event(kbd->kbd_input, EV_MSC, MSC_SCAN, keycode);
		input_report_key(kbd->kbd_input, key, true);
	} else {
		/* On release, use the same key that was pressed to avoid mismatch */
		key = kbd->last_key_pressed[keycode];
		dev_info(&kbd->client->dev, "  -> Lookup last_key_pressed[%d] = %d\n", keycode, key);
		if (key == 0) {
			/* If we don't have a record, use current state (shouldn't happen) */
			if (fn)
				key = keymap_fn[keycode];
			else if (shift)
				key = keymap_shift[keycode];
			else
				key = keymap_normal[keycode];
		}
		
		dev_info(&kbd->client->dev, "  -> Releasing linux_keycode=%d\n", key);
		
		dev_info(&kbd->client->dev, "  -> input_report_key(dev, %d, 0)\n", key);
		input_event(kbd->kbd_input, EV_MSC, MSC_SCAN, keycode);
		input_report_key(kbd->kbd_input, key, false);
		kbd->last_key_pressed[keycode] = 0;
	}
	
	input_sync(kbd->kbd_input);
	dev_info(&kbd->client->dev, "  -> input_sync() called\n");
}

static void lyra_kbd_process_fifo(struct lyra_kbd_data *kbd)
{
	int i = 0, ret;
	u8 fifo_data;
	u8 event_type, keycode;
	bool pressed;
	
	dev_info(&kbd->client->dev, "Processing FIFO cycle start\n");
	
	/* Drain FIFO until hardware reports no more events or safety limit hit */
	while (i < FIFO_MAX_READ) {
		ret = lyra_kbd_read_reg(kbd->client, REG_FIFO_ACCESS);
		if (ret < 0)
			return;
		
		fifo_data = (u8)ret;
		event_type = fifo_data & FIFO_EVENT_TYPE_MASK;
		keycode = (fifo_data & FIFO_KEYCODE_MASK) >> FIFO_KEYCODE_SHIFT;
		
		/* event_type == FIFO_EVENT_NONE means FIFO empty */
		if (event_type == FIFO_EVENT_NONE)
			break;
		
		/* Debug: log every FIFO read */
		dev_info(&kbd->client->dev, "FIFO[%d]: raw=0x%02x type=%d code=%d\n",
			 i, fifo_data, event_type, keycode);
		
		switch (event_type) {
		case FIFO_EVENT_PRESS:
			pressed = true;
			break;
		case FIFO_EVENT_RELEASE:
			pressed = false;
			break;
		case FIFO_EVENT_HOLD:
			/*
			 * Ignore HOLD events - let kernel handle auto-repeat.
			 * Processing HOLD as PRESS can cause stuck keys if
			 * release events are missed, especially for direct
			 * GPIO keys (FN1-FN8).
			 */
			dev_info(&kbd->client->dev, "  -> Ignoring HOLD event for keycode %d\n", keycode);
			goto next;
		default:
			dev_warn(&kbd->client->dev, "  -> Unknown event type: %d (raw=0x%02x)\n",
				 event_type, fifo_data);
			goto next;
		}
		
		lyra_kbd_process_key_event(kbd, keycode, pressed);

		/* safety label to keep single exit for each loop iteration */
next:
		i++;
	}
	if (i > 0)
		dev_info(&kbd->client->dev, "FIFO processing done. Processed %d events.\n", i);
}

static void lyra_kbd_process_mouse(struct lyra_kbd_data *kbd)
{
	int ret;
	s8 delta_x, delta_y;
	s32 adjusted_x, adjusted_y;
	
	/* Read mouse X delta */
	ret = lyra_kbd_read_reg(kbd->client, REG_MOUSE_X);
	if (ret < 0)
		return;
	delta_x = (s8)ret;
	
	/* Read mouse Y delta */
	ret = lyra_kbd_read_reg(kbd->client, REG_MOUSE_Y);
	if (ret < 0)
		return;
	delta_y = (s8)ret;
	
	/* Apply speed multiplier */
	if (delta_x != 0) {
		adjusted_x = ((s32)delta_x * kbd->mouse_speed_x) / 100;
		if (adjusted_x == 0 && delta_x != 0)
			adjusted_x = (delta_x > 0) ? 1 : -1;
		input_report_rel(kbd->mouse_input, REL_X, adjusted_x);
	}
	
	if (delta_y != 0) {
		adjusted_y = ((s32)delta_y * kbd->mouse_speed_y) / 100;
		if (adjusted_y == 0 && delta_y != 0)
			adjusted_y = (delta_y > 0) ? 1 : -1;
		input_report_rel(kbd->mouse_input, REL_Y, adjusted_y);
	}
	
	if (delta_x != 0 || delta_y != 0)
		input_sync(kbd->mouse_input);
}

static void lyra_kbd_process_power_button(struct lyra_kbd_data *kbd, bool pressed)
{
	if (kbd->power_btn_pressed != pressed) {
		kbd->power_btn_pressed = pressed;
		input_report_key(kbd->kbd_input, KEY_POWER, pressed);
		input_sync(kbd->kbd_input);
		dev_info(&kbd->client->dev, "Power button %s\n", 
			 pressed ? "pressed" : "released");
	}
}

static void lyra_kbd_sync_modifiers(struct lyra_kbd_data *kbd)
{
	int ret;
	u8 key_status;
	bool shift, alt;

	ret = lyra_kbd_read_reg(kbd->client, REG_KEY_STATUS);
	if (ret < 0)
		return;

	key_status = (u8)ret;
	shift = (key_status & KEY_STATUS_SHIFT_BIT) != 0;
	alt = (key_status & KEY_STATUS_ALT_BIT) != 0;

	/* 
	 * Report modifier state based on register, not FIFO events.
	 * We map the single SHIFT bit to KEY_LEFTSHIFT.
	 */
	input_report_key(kbd->kbd_input, KEY_LEFTSHIFT, shift);
	input_report_key(kbd->kbd_input, KEY_LEFTALT, alt);
	input_sync(kbd->kbd_input);
	
	dev_dbg(&kbd->client->dev, "Synced modifiers: shift=%d alt=%d\n", shift, alt);
}

static void lyra_kbd_poll_work(struct work_struct *work)
{
	struct lyra_kbd_data *kbd = container_of(work, struct lyra_kbd_data,
						  poll_work.work);
	int ret;
	u8 int_status;
	
	/* Read interrupt status */
	ret = lyra_kbd_read_reg(kbd->client, REG_INT_STATUS);
	if (ret < 0)
		goto reschedule;
	
	int_status = (u8)ret;
	
	/* Sync modifiers if hardware reports a change */
	if (int_status & (INT_STATUS_SHIFT_CHANGE | INT_STATUS_ALT_CHANGE | INT_STATUS_FN_CHANGE))
		lyra_kbd_sync_modifiers(kbd);

	/* Check for FIFO overflow */
	if (int_status & INT_STATUS_FIFO_OVERFLOW)
		dev_warn(&kbd->client->dev, "FIFO overflow detected\n");
	
	/* Process keyboard events */
	if (int_status & INT_STATUS_KEY_EVENT)
		lyra_kbd_process_fifo(kbd);
	
	/* Process mouse events */
	if (int_status & INT_STATUS_MOUSE_EVENT)
		lyra_kbd_process_mouse(kbd);
	
	/* Process power button */
	if (int_status & INT_STATUS_POWER_BTN) {
		/* Read key status to get current power button state */
		/* Assuming power button state is tracked separately by firmware */
		/* For now, toggle on each power button interrupt */
		lyra_kbd_process_power_button(kbd, !kbd->power_btn_pressed);
	}
	
reschedule:
	/* Reschedule work */
	schedule_delayed_work(&kbd->poll_work,
			      msecs_to_jiffies(kbd->poll_interval_ms));
}

/* Sysfs attributes for mouse speed */
static ssize_t mouse_speed_x_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	
	return sprintf(buf, "%d\n", kbd->mouse_speed_x);
}

static ssize_t mouse_speed_x_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	int val;
	
	if (kstrtoint(buf, 10, &val))
		return -EINVAL;
	
	if (val < 10 || val > 500)
		return -EINVAL;
	
	kbd->mouse_speed_x = val;
	
	return count;
}

static ssize_t mouse_speed_y_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	
	return sprintf(buf, "%d\n", kbd->mouse_speed_y);
}

static ssize_t mouse_speed_y_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	int val;
	
	if (kstrtoint(buf, 10, &val))
		return -EINVAL;
	
	if (val < 10 || val > 500)
		return -EINVAL;
	
	kbd->mouse_speed_y = val;
	
	return count;
}

static ssize_t poll_interval_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	
	return sprintf(buf, "%u\n", kbd->poll_interval_ms);
}

static ssize_t poll_interval_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	unsigned int val;
	
	if (kstrtouint(buf, 10, &val))
		return -EINVAL;
	
	if (val < 5 || val > 100)
		return -EINVAL;
	
	kbd->poll_interval_ms = val;
	
	return count;
}

static DEVICE_ATTR_RW(mouse_speed_x);
static DEVICE_ATTR_RW(mouse_speed_y);
static DEVICE_ATTR_RW(poll_interval);

static struct attribute *lyra_kbd_attrs[] = {
	&dev_attr_mouse_speed_x.attr,
	&dev_attr_mouse_speed_y.attr,
	&dev_attr_poll_interval.attr,
	NULL,
};

static const struct attribute_group lyra_kbd_attr_group = {
	.attrs = lyra_kbd_attrs,
};

static int lyra_kbd_setup_input_devices(struct lyra_kbd_data *kbd)
{
	struct i2c_client *client = kbd->client;
	struct input_dev *kbd_input, *mouse_input;
	int i, error;
	
	/* Allocate keyboard input device */
	kbd_input = devm_input_allocate_device(&client->dev);
	if (!kbd_input)
		return -ENOMEM;
	
	kbd_input->name = "Luckfox Lyra Keyboard";
	kbd_input->phys = "i2c-keyboard/input0";
	kbd_input->id.bustype = BUS_I2C;
	kbd_input->id.vendor = 0x1234;
	kbd_input->id.product = 0x5678;
	kbd_input->id.version = 0x0100;
	
	/* Set keyboard capabilities */
	__set_bit(EV_KEY, kbd_input->evbit);
	__set_bit(EV_REP, kbd_input->evbit);
	__set_bit(EV_MSC, kbd_input->evbit);
	__set_bit(MSC_SCAN, kbd_input->mscbit);
	
	/* Set all possible keys from keymaps */
	for (i = 0; i < MAX_KEYCODES; i++) {
		__set_bit(keymap_normal[i], kbd_input->keybit);
		__set_bit(keymap_shift[i], kbd_input->keybit);
		__set_bit(keymap_fn[i], kbd_input->keybit);
	}
	
	/* Add power button */
	__set_bit(KEY_POWER, kbd_input->keybit);
	
	error = input_register_device(kbd_input);
	if (error) {
		dev_err(&client->dev, "Failed to register keyboard: %d\n", error);
		return error;
	}
	
	kbd->kbd_input = kbd_input;
	
	/* Allocate mouse input device */
	mouse_input = devm_input_allocate_device(&client->dev);
	if (!mouse_input)
		return -ENOMEM;
	
	mouse_input->name = "Luckfox Lyra Mouse";
	mouse_input->phys = "i2c-keyboard/input1";
	mouse_input->id.bustype = BUS_I2C;
	mouse_input->id.vendor = 0x1234;
	mouse_input->id.product = 0x5679;
	mouse_input->id.version = 0x0100;
	
	/* Set mouse capabilities */
	__set_bit(EV_REL, mouse_input->evbit);
	__set_bit(REL_X, mouse_input->relbit);
	__set_bit(REL_Y, mouse_input->relbit);
	__set_bit(REL_WHEEL, mouse_input->relbit);
	
	__set_bit(EV_KEY, mouse_input->evbit);
	__set_bit(BTN_LEFT, mouse_input->keybit);
	__set_bit(BTN_RIGHT, mouse_input->keybit);
	__set_bit(BTN_MIDDLE, mouse_input->keybit);
	
	error = input_register_device(mouse_input);
	if (error) {
		dev_err(&client->dev, "Failed to register mouse: %d\n", error);
		return error;
	}
	
	kbd->mouse_input = mouse_input;
	
	return 0;
}

static int lyra_kbd_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct lyra_kbd_data *kbd;
	int error;
	
	/* Check I2C functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "I2C adapter doesn't support SMBUS_BYTE_DATA\n");
		return -EIO;
	}
	
	/* Allocate driver data */
	kbd = devm_kzalloc(&client->dev, sizeof(*kbd), GFP_KERNEL);
	if (!kbd)
		return -ENOMEM;
	
	kbd->client = client;
	kbd->mouse_speed_x = 100; /* 1x speed */
	kbd->mouse_speed_y = 100;
	kbd->poll_interval_ms = POLL_INTERVAL_MS;
	
	i2c_set_clientdata(client, kbd);
	
	/* Setup input devices */
	error = lyra_kbd_setup_input_devices(kbd);
	if (error)
		return error;
	
	/* Create sysfs attributes */
	error = sysfs_create_group(&client->dev.kobj, &lyra_kbd_attr_group);
	if (error) {
		dev_err(&client->dev, "Failed to create sysfs group: %d\n", error);
		return error;
	}
	
	/* Initialize and start polling work */
	INIT_DELAYED_WORK(&kbd->poll_work, lyra_kbd_poll_work);
	
	/* Initial modifier sync */
	lyra_kbd_sync_modifiers(kbd);

	schedule_delayed_work(&kbd->poll_work,
			      msecs_to_jiffies(kbd->poll_interval_ms));
	
	dev_info(&client->dev, "Luckfox Lyra keyboard/mouse initialized\n");
	
	return 0;
}

static void lyra_kbd_remove(struct i2c_client *client)
{
	struct lyra_kbd_data *kbd = i2c_get_clientdata(client);
	
	/* Cancel polling work */
	cancel_delayed_work_sync(&kbd->poll_work);
	
	/* Remove sysfs attributes */
	sysfs_remove_group(&client->dev.kobj, &lyra_kbd_attr_group);
}

static int __maybe_unused lyra_kbd_suspend(struct device *dev)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	
	/* Stop polling during suspend */
	cancel_delayed_work_sync(&kbd->poll_work);
	
	return 0;
}

static int __maybe_unused lyra_kbd_resume(struct device *dev)
{
	struct lyra_kbd_data *kbd = dev_get_drvdata(dev);
	
	/* Resume polling */
	schedule_delayed_work(&kbd->poll_work,
			      msecs_to_jiffies(kbd->poll_interval_ms));
	
	return 0;
}

static SIMPLE_DEV_PM_OPS(lyra_kbd_pm_ops, lyra_kbd_suspend, lyra_kbd_resume);

static const struct of_device_id lyra_kbd_of_match[] = {
	{ .compatible = "luckfox,lyra-keyboard" },
	{ }
};
MODULE_DEVICE_TABLE(of, lyra_kbd_of_match);

static const struct i2c_device_id lyra_kbd_id[] = {
	{ "lyra-keyboard", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lyra_kbd_id);

static struct i2c_driver lyra_kbd_driver = {
	.driver = {
		.name	= "lyra-i2c-keyboard",
		.of_match_table = lyra_kbd_of_match,
		.pm	= &lyra_kbd_pm_ops,
	},
	.probe		= lyra_kbd_probe,
	.remove		= lyra_kbd_remove,
	.id_table	= lyra_kbd_id,
};

module_i2c_driver(lyra_kbd_driver);

MODULE_AUTHOR("Luckfox");
MODULE_DESCRIPTION("Luckfox Lyra I2C Keyboard and Mouse Driver");
MODULE_LICENSE("GPL v2");
