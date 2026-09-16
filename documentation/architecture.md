# System Architecture — Smart DIY Stream Deck

```
                    SMART DIY STREAM DECK
                            |
             +--------------+--------------+
             |                             |
        ATmega32U4                     Windows PC
             |                             |
      +------+-------+              +------+-------+
      |      |       |              |              |
    Keys   OLED   Encoder       AutoHotkey    AutoHotkey
      |      |       |          (launcher)   (PC stats)
      |      |       |              |              |
      |      |       +----------> Volume       CPU/RAM/Time
      |      |                             |
      +------+-----------------------------+
                     USB (HID + Serial/CDC)
```

## Layers

### 1. Hardware layer (ATmega32U4 Pro Micro)

- **Keys** — 9 push buttons, read with `INPUT_PULLUP`, debounced in firmware. A key press triggers an OLED icon animation and sends a `Ctrl+Alt+N` USB HID shortcut.
- **OLED** — 128×32 I2C display. Shows startup message, per-key icon/label, volume level, mute status, and (when idle) PC time/CPU/RAM.
- **Rotary encoder** — quadrature-decoded in firmware using a transition table; drives `Consumer.write(MEDIA_VOLUME_UP/DOWN)` for volume, and toggles mute on push.

### 2. PC software layer (Windows, AutoHotkey)

- **`macropad_launcher.ahk`** — binds `Ctrl+Alt+1..9` to `Run()` calls that open the mapped application or website. Handles manual path overrides and automatic executable detection for a few desktop apps.
- **`macropad_pc_stats.ahk`** — a fully separate, independent script. On a timer, it queries Windows for the current time, CPU load, and RAM usage via WMI, then writes a single line (`HH:MM|CPU|RAM`) to the Pro Micro's COM port at 9600 baud.

These two scripts do not share code or state — this was a deliberate design decision after an earlier combined script caused instability (see [`troubleshooting.md`](troubleshooting.md)).

### 3. USB link

The ATmega32U4's native USB exposes two interfaces over one physical cable:

- **HID** — keyboard shortcuts + consumer control (volume/mute), via the HID-Project library.
- **Serial/CDC** — a virtual COM port used only for the one-way PC → firmware idle-stats feed.

## Data flow: pressing a macro key

```
Physical key press
   → ATmega32U4 detects the press (debounced)
   → OLED animates the app icon + label
   → Firmware sends Ctrl+Alt+N over USB HID
   → macropad_launcher.ahk receives the hotkey
   → Application/website launches on Windows
```

## Data flow: idle screen

```
macropad_pc_stats.ahk (every ~2s)
   → Queries Windows time, CPU load (WMI), RAM usage (WMI)
   → Writes "HH:MM|CPU|RAM\n" to COM7 at 9600 baud
   → Firmware reads the line over Serial (non-blocking)
   → Firmware parses time/CPU/RAM into internal variables
   → After ~30s of key/encoder inactivity, OLED shows this data
   → If no fresh data arrives within the timeout window, OLED turns off instead
```
