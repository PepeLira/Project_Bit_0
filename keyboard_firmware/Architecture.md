# Architecture

## High-Level Overview
- **Target**: RP2040 running the Pico SDK, exposing an I²C keyboard with a soft power button and status NeoPixel.
- **Peripherals**: GPIO matrix/discrete keys (future work), dedicated power latch input, single NeoPixel driven through PIO, optional I²C + interrupt once FIFO protocol is implemented.
- **Runtime Model**: `main.c` brings up stdio, GPIO, NeoPixel, and the power-button service, then yields to a cooperative loop that polls the power logic every millisecond while the NeoPixel subsystem drives feedback colors.

```
┌──────────┐    config.h      ┌──────────────┐
│ main.c   ├─────────────►    │ GPIO/PIO HAL │
└────▲─────┘                  └──────┬───────┘
     │                              │
     │ calls                        │
┌────┴─────────────┐        ┌───────┴─────────┐
│ power_button.c   │◄──────►│ neopixel.c      │
└──────────────────┘ emits  └─────────────────┘
       ▲ status colors via rgb_color_t structs
       │
   Human button input (GPIO POWER_BUTTON)
```

## Execution Flow
1. **Boot** (`src/main.c`):
   - Initialize stdio for logging and wait for USB stability.
   - Configure the power-button GPIO with pull-up so a pressed key reads low.
2. **Subsystem bring-up**:
   - `neopixel_init()` installs the `ws2812_program` into PIO0 and claims an unused state machine.
   - `power_button_init()` resets the internal state machine that tracks hold durations.
3. **Main loop**:
   - `power_button_update()` polls GPIO, enforcing debounce/hold logic derived from `config.h` timing constants.
   - When the button enters the “holding” state it modulates the NeoPixel between `DEFAULT_COLOR` and `SHUTDOWN_COLOR`. After `POWER_HOLD_MS` it actively discharges the power rail (drives GPIO low) until a release is observed.

## Module and File Responsibilities
- `src/main.c`: Firmware entry point, prints startup diagnostics, prepares GPIO direction/pulls, invokes subsystem init, and schedules the 1 kHz `power_button_update()` task.
- `src/power_button.c`: Implements the three-state power controller (`IDLE → HOLDING → DISCHARGED`), toggles GPIO direction to latch power, samples release windows, and orchestrates NeoPixel cues.
- `src/neopixel.c`: Wraps the `ws2812.pio` program, handles state-machine allocation, and exposes helpers to push GRB data frames for a single LED.
- `src/neopixel.pio`: PIO assembly for WS2812 signaling; exports `ws2812_program_init()` that the C layer calls to configure clock dividers, pin directions, and shift behavior.
- `include/config.h`: Centralizes pin assignments pulled from `keyboard_layout.json`, defines debounce/hold/breathing constants, and declares the `rgb_color_t` convenience struct used by both firmware modules.
- `include/neopixel.h`: Declares the NeoPixel API surface so that both `main.c` and `power_button.c` can request color changes without knowing PIO internals.
- `include/power_button.h`: Declares the `power_state_t` enum plus service APIs (`*_init`, `*_update`, `power_button_get_state`) for any caller that needs to monitor power transitions.
- `keyboard_layout.json`: User-editable description of the physical layout whose GPIO mappings inform `config.h` (currently a placeholder for future matrix scanning logic).
- `CMakeLists.txt`: Defines the Pico SDK target, includes PIO sources, and ensures the UF2 binary embeds bi_decl metadata exposed in `main.c`.

### Header Contracts

#### `include/config.h`
```c
// Pinout
#define I2C_SCL 0
#define I2C_SDA 1
#define POWER_BUTTON 29
#define NEOPIXEL_GPIO 28

// Power-button timing
#define DEBOUNCE_MS 20
#define POWER_HOLD_MS 3000
#define BLINK_PERIOD_MS 1000
#define BLINK_ON_MS 500
#define DISCHARGE_SAMPLE_MS 5

typedef struct {
    uint32_t r;
    uint32_t g;
    uint32_t b;
} rgb_color_t;

#define NEOPIXEL_COUNT 1
#define DEFAULT_COLOR  ((rgb_color_t){0, 5, 0})
#define SHUTDOWN_COLOR ((rgb_color_t){5, 0, 0})
#define MODIFIER_COLOR ((rgb_color_t){5, 5, 0})
```

#### `include/neopixel.h`
```c
typedef enum {
    COLOR_GREEN = 0,
    COLOR_RED   = 1,
    COLOR_OFF   = 2
} neopixel_color_t;

void neopixel_init(void);
void neopixel_set_color(uint32_t r, uint32_t g, uint32_t b);
void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void neopixel_off(void);
```
> _Note_: `src/neopixel.c` currently implements `neopixel_set_color(rgb_color_t color)`, so aligning the header or adding a wrapper will be necessary.

#### `include/power_button.h`
```c
typedef enum {
    POWER_STATE_IDLE,
    POWER_STATE_HOLDING,
    POWER_STATE_DISCHARGED
} power_state_t;

void power_button_init(void);
void power_button_update(void);
power_state_t power_button_get_state(void);
void gpio_reset_to_default(uint pin);
```

## Data and Control Coupling
- `power_button.c` consumes `POWER_HOLD_MS`, `BLINK_*`, and the color macros from `config.h` while driving the `POWER_BUTTON` GPIO defined there, guaranteeing a single configuration source.
- `neopixel.c` respects `NEOPIXEL_GPIO` and `NEOPIXEL_COUNT` (currently 1) from `config.h`, so swapping pins or colors does not require touching the PIO logic.
- `main.c` acts as the glue layer: it initializes per-pin pulls and defers specialized behavior to the modules, which keeps board-specific adjustments localized in `config.h` or future layout files.

## Extensibility Notes
- The current loop occupies little CPU time, leaving ample headroom to add the I²C FIFO service described in `README.md`. Hooks would live beside `power_button_update()` in `main.c`.
- Because the NeoPixel driver uses the Pico SDK’s PIO helper, extending to more LEDs only requires adjusting `NEOPIXEL_COUNT` and repeating `pio_sm_put_blocking()` calls per LED.
- The `keyboard_layout.json → config.h` flow already anticipates matrix scanning; once implemented, new headers (e.g., `matrix_scan.h`) should follow the same contract-based documentation style outlined above.
