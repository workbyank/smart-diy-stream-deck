# Software

Two independent AutoHotkey (v2) scripts run on the PC side. They are kept fully separate on purpose — see [`documentation/troubleshooting.md`](../documentation/troubleshooting.md), Issue 2.

## `macropad_launcher.ahk`

Listens for `Ctrl+Alt+1` through `Ctrl+Alt+9` (sent by the firmware) and launches the mapped application or website. Key → app/website assignments live in the `Targets` map near the top of the file — no firmware changes are needed to reassign a key.

Includes:
- Direct URL launching for web-based targets (ChatGPT, YouTube, Claude, Onshape, Spotify's `spotify:` URI)
- Automatic executable detection for a few desktop apps (Chrome, Arduino IDE)
- A manual path override (`ManualPaths`) for apps whose install location isn't predictable — currently used for Bambu Studio

## `macropad_pc_stats.ahk`

A separate, independent script with no hotkeys. On a timer, it:
1. Queries Windows for CPU load and RAM usage via WMI, and reads the current time
2. Opens the Pro Micro's COM port (`COM7` by default — **machine-specific**, change `StatsPort` to match your system)
3. Writes one line: `HH:MM|CPU|RAM\n` at 9600 baud, roughly every 2 seconds

> **Note:** despite being described conceptually as a "PC statistics sender," this is implemented in **AutoHotkey**, not PowerShell — this matches what was actually built and tested for this project.

## `Macropad_PC_Stats.lnk`

A Windows shortcut used to launch `macropad_pc_stats.ahk`, intended to be placed in the Windows Startup folder so the PC-stats sender starts automatically on login.

## Requirements

- [AutoHotkey v2](https://www.autohotkey.com/)
- Windows (both scripts use Windows-specific APIs — WMI for stats, `Run()` for launching)
