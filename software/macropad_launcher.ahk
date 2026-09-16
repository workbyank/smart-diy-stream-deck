#Requires AutoHotkey v2.0
#SingleInstance Force

; ============================================================
; MACROPAD LAUNCHER - STABLE VERSION
; Ctrl+Alt+1 ... Ctrl+Alt+9
; ============================================================

Targets := Map(
    "1", Map("label", "ChatGPT",       "type", "url", "value", "https://chatgpt.com"),
    "2", Map("label", "Bambu Studio",  "type", "exe", "value", "auto"),
    "3", Map("label", "Spotify",       "type", "url", "value", "spotify:"),
    "4", Map("label", "YouTube",       "type", "url", "value", "https://www.youtube.com"),
    "5", Map("label", "Chrome",        "type", "exe", "value", "auto"),
    "6", Map("label", "Claude",        "type", "url", "value", "https://claude.ai"),
    "7", Map("label", "Arduino IDE",   "type", "exe", "value", "auto"),
    "8", Map("label", "File Explorer", "type", "exe", "value", "explorer.exe"),
    "9", Map("label", "Onshape",       "type", "url", "value", "https://cad.onshape.com")
)

; ============================================================
; MANUAL PATHS
; ============================================================

ManualPaths := Map(
    "Bambu Studio", "D:\Bambu Studio\bambu-studio.exe"
)

; ============================================================
; WINDOWS FOLDERS
; ============================================================

LocalAppData := EnvGet("LOCALAPPDATA")
AppData := EnvGet("APPDATA")
ProgramFiles := EnvGet("ProgramFiles")
ProgramFilesX86 := EnvGet("ProgramFiles(x86)")

; ============================================================
; AUTO DETECTION
; ============================================================

AutoDetect := Map()

AutoDetect["Spotify"] := [
    AppData "\Spotify\Spotify.exe",
    LocalAppData "\Microsoft\WindowsApps\Spotify.exe"
]

AutoDetect["Chrome"] := [
    ProgramFiles "\Google\Chrome\Application\chrome.exe",
    LocalAppData "\Google\Chrome\Application\chrome.exe"
]

if (ProgramFilesX86 != "")
    AutoDetect["Chrome"].Push(
        ProgramFilesX86 "\Google\Chrome\Application\chrome.exe"
    )

AutoDetect["Arduino IDE"] := [
    LocalAppData "\Programs\Arduino IDE\Arduino IDE.exe",
    ProgramFiles "\Arduino IDE\Arduino IDE.exe",
    ProgramFiles "\Arduino\arduino.exe"
]

if (ProgramFilesX86 != "") {
    AutoDetect["Arduino IDE"].Push(
        ProgramFilesX86 "\Arduino IDE\Arduino IDE.exe"
    )
    AutoDetect["Arduino IDE"].Push(
        ProgramFilesX86 "\Arduino\arduino.exe"
    )
}

; ============================================================
; LAUNCH FUNCTION
; ============================================================

Launch(key) {
    global Targets, ManualPaths, AutoDetect

    try {
        t := Targets[key]
        label := t["label"]
        type := t["type"]
        value := t["value"]

        ; Website / URL
        if (type = "url") {
            Run(value)
            return
        }

        ; Manual path
        if (ManualPaths.Has(label)) {
            path := ManualPaths[label]

            if FileExist(path) {
                Run('"' path '"')
                return
            }

            TrayTip(
                "Macropad Launcher",
                "File not found:`n" path,
                5
            )
            return
        }

        ; Direct program
        if (value != "auto" && value != "manual") {
            try {
                Run(value)
                return
            }
        }

        ; Auto detection
        if AutoDetect.Has(label) {
            for path in AutoDetect[label] {
                if FileExist(path) {
                    Run('"' path '"')
                    return
                }
            }
        }

        TrayTip(
            "Macropad Launcher",
            label " was not found on this PC.",
            5
        )

    } catch as e {
        TrayTip(
            "Macropad Launcher",
            "Launch error:`n" e.Message,
            5
        )
    }
}

; ============================================================
; MACROPAD HOTKEYS
; ============================================================

^!1::Launch("1")
^!2::Launch("2")
^!3::Launch("3")
^!4::Launch("4")
^!5::Launch("5")
^!6::Launch("6")
^!7::Launch("7")
^!8::Launch("8")
^!9::Launch("9")
