# Troubleshooting & Debugging History

Real issues encountered during development, kept here honestly rather than hidden.

## Issue 1 — HID-Project board error

**Symptom:** `HID Project can only be used with an USB MCU`

**Cause:** Wrong board profile selected in the Arduino IDE — a non-USB board configuration was selected instead of a USB-capable one.

**Resolution:** Select a genuine USB-capable ATmega32U4 board profile (e.g. Arduino Leonardo / SparkFun Pro Micro configuration), not a classic non-USB Pro Mini-style profile.

## Issue 2 — Combined launcher + PC-stats script instability

**Symptom:** Running application launching and serial PC-stats communication from within the same AutoHotkey script caused instability.

**Resolution:** Split the functionality into two fully independent scripts — `macropad_launcher.ahk` (hotkeys only) and `macropad_pc_stats.ahk` (serial telemetry only) — with no shared state between them.

## Issue 3 — COM port is machine-specific

**Symptom:** The PC-stats script is hardcoded to `COM7`.

**Explanation:** `COM7` was simply the port Windows assigned to the Pro Micro on the development machine. COM port numbers are assigned by Windows and **will very likely differ on another PC**. Check Device Manager → Ports (COM & LPT) and update the `StatsPort` value in `macropad_pc_stats.ahk` accordingly.

## Issue 4 — Stats/launcher window appearing on screen

**Symptom:** Running the scripts could show a visible window or flash a console.

**Resolution:** Run both AutoHotkey scripts via a Windows Startup shortcut so they launch hidden in the background — AutoHotkey scripts don't show a console window by default when run this way.

## Issue 5 — OLED going blank after inactivity

**Symptom:** After a period of inactivity, the OLED turns off.

**Cause:** This is intentional firmware behavior — the OLED only shows the idle time/CPU/RAM screen while **fresh** PC-stats data is being received. If no fresh data has arrived within the configured timeout, the firmware turns the display off rather than showing stale, frozen information.

## Issue 6 — Serial Monitor conflict

**Symptom:** The PC-stats script fails to open the COM port.

**Cause:** The Arduino IDE's Serial Monitor or Serial Plotter was left open, holding the same COM port.

**Resolution:** Close the Serial Monitor/Plotter before running `macropad_pc_stats.ahk` — only one program can hold a COM port open at a time.

## Issue 7 — Bambu Studio executable path

**Symptom:** Bambu Studio wouldn't launch via automatic detection.

**Cause:** Bambu Studio's install location isn't always predictable across machines.

**Resolution:** A manual path override (`ManualPaths` map in `macropad_launcher.ahk`) is used instead of relying solely on auto-detection. The path used during development — `D:\Bambu Studio\bambu-studio.exe` — is specific to that machine and will need to be changed for any other setup.
