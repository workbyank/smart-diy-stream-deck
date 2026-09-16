# Pinout — Smart DIY Stream Deck

Controller: **ATmega32U4 Pro Micro (5V / 16MHz)**

Exact silkscreen labels can vary slightly between Pro Micro board variants/clones — match by pin function (e.g. "A0", "A1", "SDA") rather than by physical position if your board differs.

---

## OLED (I2C)

| OLED Pin | Pro Micro Pin | Notes |
|---|---|---|
| SDA | 2 (SDA) | I2C data |
| SCL | 3 (SCL) | I2C clock |
| VCC | VCC | 5V |
| GND | GND | Ground |

- Display: 0.91", 128×32, I2C
- I2C address used in firmware: `0x3C`
- Firmware runs I2C at 400kHz (`Wire.setClock(400000)`) for faster redraws

---

## Rotary Encoder

| Encoder Pin | Pro Micro Pin | Notes |
|---|---|---|
| A / CLK | A1 | Quadrature signal A |
| B / DT | A0 | Quadrature signal B |
| SW | A2 | Push-button (mute/unmute) |
| VCC | VCC | 5V |
| GND | GND | Ground |

All three encoder pins (`A1`, `A0`, `A2`) are configured `INPUT_PULLUP` in firmware.

---

## Macro Keys (9x)

All 9 keys are configured `INPUT_PULLUP` — each switch connects its pin to GND when pressed (active LOW).

| Key | Pro Micro Pin |
|---|---|
| K1 | 4 |
| K2 | 14 |
| K3 | 16 |
| K4 | 5 |
| K5 | 6 |
| K6 | 7 |
| K7 | 8 |
| K8 | 9 |
| K9 | 10 |

Firmware array: `KEY_PINS[9] = {4, 14, 16, 5, 6, 7, 8, 9, 10}`

---

## USB

The ATmega32U4 provides native USB, exposing two logical interfaces over the **same USB cable**:

| Interface | Purpose |
|---|---|
| USB HID (Keyboard + Consumer Control) | Sends `Ctrl+Alt+1..9` macro shortcuts and media/volume commands (via HID-Project) |
| USB Serial / CDC | Carries PC time/CPU/RAM data from the PC-stats script to the firmware (9600 baud) |

These are separate logical channels handled by the same USB stack — they don't interfere with each other.
