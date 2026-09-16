# Setup Guide — Smart DIY Stream Deck

## 1. Hardware Wiring

Wire the OLED, rotary encoder, and 9 push buttons to the Pro Micro exactly as described in [`hardware/pinout.md`](../hardware/pinout.md) and shown in [`hardware/circuit-diagram.png`](../hardware/circuit-diagram.png).

## 2. Arduino IDE Setup

1. Install the [Arduino IDE](https://www.arduino.cc/en/software).
2. Install the required libraries via **Sketch → Include Library → Manage Libraries**:
   - `HID-Project`
   - `Adafruit GFX Library`
   - `Adafruit SSD1306`
3. **Board selection matters:** select a genuine USB-capable ATmega32U4 profile (e.g. **Arduino Leonardo** or **SparkFun Pro Micro**, matching your specific board). A non-USB board profile (like a classic Pro Mini configuration) will cause HID-Project to fail — see [`troubleshooting.md`](troubleshooting.md), Issue 1.

## 3. Flashing the Firmware

1. Open [`firmware/smart_stream_deck.ino`](../firmware/smart_stream_deck.ino) in the Arduino IDE.
2. Select the correct board and COM port.
3. Upload.
4. Once uploaded, **the Arduino IDE does not need to stay open** — the device runs independently once plugged into any USB port, powered or not connected to the IDE.

## 4. AutoHotkey Setup

1. Install [AutoHotkey v2](https://www.autohotkey.com/).
2. Run [`software/macropad_launcher.ahk`](../software/macropad_launcher.ahk) by double-clicking it. A tray icon confirms it's running.
3. Test a key press — the mapped application/website should open.
4. If an app doesn't open (e.g. it's installed somewhere non-standard), edit the `ManualPaths` map at the top of the script with the correct `.exe` path.

## 5. PC Stats Sender Setup

1. Run [`software/macropad_pc_stats.ahk`](../software/macropad_pc_stats.ahk) separately from the launcher.
2. **Find your Pro Micro's COM port** in Windows Device Manager → Ports (COM & LPT). The script currently targets `COM7` — this was the port used during development and **will likely be different on your PC**. Edit the `StatsPort` variable at the top of the script to match.
3. Once running, wait ~30 seconds without touching the macropad — the OLED should switch to the idle time/CPU/RAM screen.

## 6. Windows Startup (Optional)

To have both scripts run automatically on login, without needing to double-click them each time:

1. Press `Win+R`, type `shell:startup`, press Enter.
2. Create shortcuts to `macropad_launcher.ahk` and `macropad_pc_stats.ahk`, and place them in this folder.
3. The Pro Micro must remain connected via USB for either script to function.

The Arduino IDE does **not** need to be part of Windows Startup — only the two AutoHotkey scripts.

## 7. Testing

Once everything is wired and running:

- Press each of the 9 keys and confirm the correct app/website opens and the OLED shows the right icon.
- Rotate the encoder both directions and confirm system volume changes.
- Press the encoder and confirm mute/unmute toggles.
- Leave the macropad idle for ~30 seconds and confirm the OLED shows time/CPU/RAM (with the PC-stats script running).
- Close the PC-stats script and confirm the OLED falls back to turning off after the idle timeout, rather than showing frozen data.
