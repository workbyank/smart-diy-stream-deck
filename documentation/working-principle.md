# Working Principle — Smart DIY Stream Deck

Step-by-step behavior from power-on:

1. Device powers on through USB.
2. The ATmega32U4 initializes — pins, USB HID (Keyboard + Consumer), and USB Serial.
3. The OLED initializes over I2C and shows **"Macropad Ready"**.
4. The 9 key pins and 3 encoder pins are configured as `INPUT_PULLUP`.
5. USB HID becomes active — the PC recognizes the device as a keyboard/consumer-control HID device, plus a serial (CDC) port.
6. **User presses a macro key.**
7. Firmware detects the press, animates the corresponding icon on the OLED, and sends `Ctrl+Alt+N` as a USB HID shortcut.
8. `macropad_launcher.ahk` (running on the PC) receives the shortcut.
9. The corresponding application or website launches.
10. **Turning the encoder** sends `MEDIA_VOLUME_UP` / `MEDIA_VOLUME_DOWN` consumer-control HID commands, adjusting Windows system volume; the OLED shows an internal volume-level bar.
11. **Pressing the encoder** toggles mute (`MEDIA_VOLUME_MUTE`) and updates the OLED accordingly.
12. Independently, `macropad_pc_stats.ahk` periodically collects the current time, CPU load, and RAM usage from Windows via WMI.
13. That data is sent to the Pro Micro over its COM port (9600 baud) as a single line: `HH:MM|CPU|RAM`.
14. Firmware reads this line over Serial (non-blocking) and parses the three fields.
15. After roughly 30 seconds without a key press or encoder movement, and if fresh PC data is available, the OLED switches to an idle screen showing time, CPU%, and RAM%, refreshed about once per second.
16. If PC data becomes stale (the PC-stats script isn't running, or the connection drops), the OLED turns off instead of displaying frozen, outdated information.
