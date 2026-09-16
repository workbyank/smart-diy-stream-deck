#Requires AutoHotkey v2.0
#SingleInstance Force
Persistent

; ============================================================
; MACROPAD PC STATS SENDER
; ------------------------------------------------------------
; This is a SEPARATE AHK script.
; It does NOT contain any hotkeys.
; It only sends clock + CPU + RAM to Arduino on COM7.
; ============================================================

StatsPort := "COM7"
StatsHandle := 0

; ------------------------------------------------------------
; CPU %
; ------------------------------------------------------------

GetCpuLoad() {
    try {
        total := 0
        count := 0

        for cpu in ComObjGet("winmgmts:").ExecQuery(
            "SELECT LoadPercentage FROM Win32_Processor"
        ) {
            total += cpu.LoadPercentage
            count++
        }

        if (count > 0)
            return Round(total / count)
    } catch {
    }

    return 0
}

; ------------------------------------------------------------
; RAM %
; ------------------------------------------------------------

GetRamUsedPercent() {
    try {
        for os in ComObjGet("winmgmts:").ExecQuery(
            "SELECT FreePhysicalMemory, TotalVisibleMemorySize FROM Win32_OperatingSystem"
        ) {
            total := os.TotalVisibleMemorySize
            free := os.FreePhysicalMemory

            if (total > 0)
                return Round((total - free) / total * 100)
        }
    } catch {
    }

    return 0
}

; ------------------------------------------------------------
; OPEN COM7
; ------------------------------------------------------------

OpenSerial() {
    global StatsPort, StatsHandle

    try {
        ; Configure COM7
        RunWait(
            A_ComSpec ' /C mode COM7: BAUD=9600 PARITY=N DATA=8 STOP=1',
            ,
            "Hide"
        )

        Sleep(500)

        ; Correct Windows serial-device path
        StatsHandle := FileOpen("\\.\" COM7, "rw")

        if !IsObject(StatsHandle) {
            StatsHandle := 0
            return false
        }

        ; Pro Micro / Leonardo USB serial can reset when opened
        Sleep(1500)

        return true
    } catch {
        StatsHandle := 0
        return false
    }
}

; ------------------------------------------------------------
; CLOSE COM7
; ------------------------------------------------------------

CloseSerial() {
    global StatsHandle

    try {
        if IsObject(StatsHandle)
            StatsHandle.Close()
    } catch {
    }

    StatsHandle := 0
}

; ------------------------------------------------------------
; SEND DATA
; Format:
; HH:MM|CPU|RAM
; ------------------------------------------------------------

SendStats() {
    global StatsHandle

    ; Connect if needed
    if !IsObject(StatsHandle) {
        if !OpenSerial()
            return
    }

    try {
        time := FormatTime(A_Now, "HH:mm")
        cpu := GetCpuLoad()
        ram := GetRamUsedPercent()

        data := time "|" cpu "|" ram "`n"

        StatsHandle.Write(data)
        StatsHandle.Flush()

    } catch {
        CloseSerial()
    }
}

; ------------------------------------------------------------
; START
; ------------------------------------------------------------

; First attempt after 2 seconds
SetTimer(SendStats, -2000)

; Then every 2 seconds
SetTimer(SendStats, 2000)
