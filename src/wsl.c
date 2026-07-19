#include "wsl.h"

#include <stdio.h>
#include <wchar.h>

#include "app_state.h"
#include "config.h"
#include "tray.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

static BOOL spawnHidden(const wchar_t* cmd, HANDLE* hProc) {
    STARTUPINFOW        startupInfo = {0};
    PROCESS_INFORMATION processInfo = {0};

    startupInfo.cb          = sizeof(startupInfo);
    startupInfo.dwFlags     = STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    // CreateProcessW requires a mutable buffer
    wchar_t buf[512];
    wcscpy_s(buf, 512, cmd);

    if (!CreateProcessW(NULL, buf, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo)) {
        return FALSE;
    }
    CloseHandle(processInfo.hThread);
    *hProc = processInfo.hProcess;
    return TRUE;
}

static void closeProc(HANDLE* processHandle) {
    if (*processHandle && *processHandle != INVALID_HANDLE_VALUE) {
        TerminateProcess(*processHandle, 0);
        WaitForSingleObject(*processHandle, PROC_TERMINATE_WAIT_MS);
        CloseHandle(*processHandle);
        *processHandle = NULL;
    }
}

// ---------------------------------------------------------------------------
// WSL sleep infinity
// ---------------------------------------------------------------------------

static void wslSleepStart(void) {
    if (hWslProc) {
        return;
    }
    wchar_t cmd[256];
    swprintf_s(cmd, 256, L"wsl -d %s sleep infinity", wslInstance);
    if (!spawnHidden(cmd, &hWslProc)) {
        hWslProc = NULL;
    }
}

static void wslSleepStop(void) {
    closeProc(&hWslProc);
}

// ---------------------------------------------------------------------------
// SSH tunnel  (ssh -A localhost)
// ---------------------------------------------------------------------------

static void sshStart(void) {
    if (hSshProc) {
        return;
    }
    wchar_t cmd[512];
    swprintf_s(cmd, 512, L"ssh -o ServerAliveInterval=15 -o ServerAliveCountMax=3 -o TCPKeepAlive=no -A localhost");
    if (!spawnHidden(cmd, &hSshProc)) {
        hSshProc = NULL;
    }
}

static void sshStop(void) {
    closeProc(&hSshProc);
}

// ---------------------------------------------------------------------------
// State machine
//
//  INACTIVE  <──click──>  ACTIVE  <──ssh-toggle──>  ACTIVE_SSH
//
// Transitioning to INACTIVE stops both processes.
// Transitioning to ACTIVE stops SSH (if running), starts WSL sleep.
// Transitioning to ACTIVE_SSH starts both (wslSleepStart is a noop if alive).
// ---------------------------------------------------------------------------

void applyState(AppState newState) {
    switch (newState) {
    case STATE_INACTIVE:
        sshStop();
        wslSleepStop();
        break;

    case STATE_ACTIVE:
        if (globalState == STATE_ACTIVE_SSH) {
            sshStop();
        }
        wslSleepStart();
        break;

    case STATE_ACTIVE_SSH:
        wslSleepStart();
        sshStart();
        break;

    default:
        break;
    }

    globalState = newState;
    updateTray();
    saveConfig();
}

// ---------------------------------------------------------------------------
// Reboot: terminate instance, then restart WSL sleep (+ SSH if active)
// ---------------------------------------------------------------------------

void wslReboot(void) {
    AppState targetState = globalState;

    // The child processes will die when the VM terminates; null them out
    // before closeProc so we don't attempt a redundant TerminateProcess.
    if (hWslProc) {
        CloseHandle(hWslProc);
        hWslProc = NULL;
    }
    if (hSshProc) {
        CloseHandle(hSshProc);
        hSshProc = NULL;
    }

    wchar_t cmd[256];
    swprintf_s(cmd, 256, L"wsl -t %s", wslInstance);
    HANDLE hTerm = NULL;
    if (spawnHidden(cmd, &hTerm)) {
        WaitForSingleObject(hTerm, WSL_TERMINATE_WAIT_MS);
        CloseHandle(hTerm);
    }

    // Restart services according to pre-reboot state
    if (targetState != STATE_INACTIVE) {
        wslSleepStart();
        if (targetState == STATE_ACTIVE_SSH) {
            sshStart();
        }
    }
}

// ---------------------------------------------------------------------------
// Process watchdog  —  called from the WM_TIMER tick
//
// Uses WaitForMultipleObjects with timeout=0 (non-blocking poll).
// If a process has exited unexpectedly it is restarted.
// ---------------------------------------------------------------------------

void checkProcesses(void) {
    HANDLE handles[2];
    DWORD  hWslIdx = (DWORD)-1;
    DWORD  hSshIdx = (DWORD)-1;
    DWORD  count   = 0;

    if (hWslProc && hWslProc != INVALID_HANDLE_VALUE) {
        hWslIdx          = count;
        handles[count++] = hWslProc;
    }
    if (hSshProc && hSshProc != INVALID_HANDLE_VALUE) {
        hSshIdx          = count;
        handles[count++] = hSshProc;
    }

    if (count == 0) {
        return;
    }

    DWORD result = WaitForMultipleObjects(count, handles, FALSE, 0);
    if (result == WAIT_TIMEOUT || result == WAIT_FAILED) {
        return;
    }

    DWORD idx = result - WAIT_OBJECT_0;

    if (idx == hWslIdx) {
        CloseHandle(hWslProc);
        hWslProc = NULL;
        if (globalState != STATE_INACTIVE) {
            wslSleepStart();
        }
    } else if (idx == hSshIdx) {
        CloseHandle(hSshProc);
        hSshProc = NULL;
        if (globalState == STATE_ACTIVE_SSH) {
            Sleep(2000);
            sshStart();
        }
    }
}

void wslCleanup(void) {
    closeProc(&hSshProc);
    closeProc(&hWslProc);
}
