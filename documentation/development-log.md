# Development Log

A chronological look at how the project came together. No specific dates are given here beyond what's independently confirmed by project files.

## Phase 1 — Concept

The goal was a compact, Stream-Deck-style productivity controller: a small physical device to launch frequently-used apps and websites with one press, plus basic system controls, without needing a commercial product.

## Phase 2 — Hardware Prototype

Core hardware assembled: an ATmega32U4 Pro Micro, a 0.91" 128×32 I2C OLED, 9 mechanical push switches, and a rotary encoder with a push switch, wired per [`hardware/pinout.md`](../hardware/pinout.md).

## Phase 3 — Firmware

Firmware built up incrementally:
- Key detection with debouncing
- USB HID shortcut output (`Ctrl+Alt+1..9`) via HID-Project
- Rotary encoder quadrature decoding for volume, and push-to-mute
- OLED UI: startup message, volume bar, mute status
- Per-key icon and drop-in animation on the OLED
- Idle-screen display (time/CPU/RAM) fed by serial data from the PC

## Phase 4 — Desktop Automation

`macropad_launcher.ahk` written to catch the `Ctrl+Alt+1..9` shortcuts and launch the mapped application or website, including automatic executable detection for a few desktop apps and a manual path override for Bambu Studio.

## Phase 5 — PC Monitoring

Serial communication added between the PC and firmware: a script queries Windows for time, CPU load, and RAM usage via WMI and forwards it to the Pro Micro, which displays it during idle periods.

## Phase 6 — Debugging

Real issues surfaced and were resolved along the way — wrong board profile tripping up HID-Project, instability from combining the launcher and PC-stats logic into one script, COM-port handling, getting the scripts to run hidden in the background, and defining clear stale-data behavior for the OLED. Full details: [`troubleshooting.md`](troubleshooting.md).

## Phase 7 — Mechanical Design

A custom 3D-printed enclosure was designed and printed to house the electronics, with Onshape used in the CAD workflow and Bambu Studio used for slicing (confirmed by the exported `3mf` file's own metadata).

## Phase 8 — Final Prototype

All pieces integrated into a single working unit: 9 macro keys, encoder-based volume/mute, OLED feedback and idle monitoring, and PC-side automation — assembled into the enclosure and in daily use.
