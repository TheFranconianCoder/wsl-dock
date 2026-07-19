#include <stdio.h>
#include <wchar.h>

#include "app_state.h"
#include "config.h"
#include "tray.h"
#include "wsl.h"

// ---------------------------------------------------------------------------
// Global definitions  (declared extern in app_state.h)
// ---------------------------------------------------------------------------

AppState        globalState = STATE_INACTIVE;
wchar_t         wslInstance[128];
NOTIFYICONDATAW notifyData = {0};
wchar_t         configPath[MAX_PATH];
wchar_t         configDir[MAX_PATH];
DWORD64         lastConfigLoad = 0;
HANDLE          hWslProc       = NULL;
HANDLE          hSshProc       = NULL;
HWND            hwnd           = NULL;

// ---------------------------------------------------------------------------
// DPI awareness
// ---------------------------------------------------------------------------

static void enableDpiAwareness(void) {
    const HMODULE H_USER32 = GetModuleHandleW(L"user32.dll");
    if (H_USER32) {
        typedef BOOL(WINAPI * SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
        // ReSharper disable once CppLocalVariableMayBeConst
        SetProcessDpiAwarenessContextProc setDpiContext =
            (SetProcessDpiAwarenessContextProc)GetProcAddress(H_USER32, "SetProcessDpiAwarenessContext");
        if (setDpiContext) {
            setDpiContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
            return;
        }
    }
    SetProcessDPIAware();
}

// ---------------------------------------------------------------------------
// Window procedure
// ---------------------------------------------------------------------------

// ReSharper disable once CppParameterMayBeConst
LRESULT CALLBACK wndProc(HWND hwnd, const UINT msg, const WPARAM wParam,
                         const LPARAM lParam) { // NOLINT(*-function-cognitive-complexity)
    switch (msg) {
    case WM_TRAYICON:
        if (lParam == WM_LBUTTONUP) {
            // Left-click: toggle INACTIVE <-> ACTIVE
            // (SSH state is preserved through ACTIVE; SSH must be toggled via menu)
            if (globalState == STATE_INACTIVE) {
                applyState(STATE_ACTIVE);
            } else {
                applyState(STATE_INACTIVE);
            }
        } else if (lParam == WM_RBUTTONUP) {
            POINT mousePt;
            GetCursorPos(&mousePt);

            // ReSharper disable once CppLocalVariableMayBeConst
            HMENU hMenu = CreatePopupMenu();

            UINT sshFlags = MF_STRING;
            if (globalState == STATE_ACTIVE_SSH) {
                sshFlags |= MF_CHECKED;
            }
            AppendMenuW(hMenu, sshFlags, ID_MENU_SSH, L"SSH-Tunnel");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_MENU_REBOOT, L"Reboot");
            AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenuW(hMenu, MF_STRING, ID_MENU_EXIT, L"Exit");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, mousePt.x, mousePt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_MENU_SSH:
            if (globalState == STATE_ACTIVE_SSH) {
                applyState(STATE_ACTIVE);
            } else {
                // Implicitly activate WSL sleep if not yet running
                applyState(STATE_ACTIVE_SSH);
            }
            break;

        case ID_MENU_REBOOT:
            wslReboot();
            break;

        case ID_MENU_EXIT:
            DestroyWindow(hwnd);
            break;
        default:
            break;
        }
        break;

    case WM_DPICHANGED:
        updateTray();
        return 0;

    case WM_TIMER:
        if (wParam == IDT_SSH_RETRY) {
            KillTimer(hwnd, IDT_SSH_RETRY);
            sshRetryTimerFired();
        }
        return 0;

    case WM_DESTROY:
        Shell_NotifyIconW(NIM_DELETE, &notifyData);
        wslCleanup();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Entry point
// ---------------------------------------------------------------------------

int main(void) { // NOLINT(*-function-cognitive-complexity)
    enableDpiAwareness();

    HANDLE hMutex = CreateMutexW(NULL, TRUE, L"WslDock_SingleInstance_Mutex");
    if (!hMutex) {
        return 1;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // ReSharper disable once CppLocalVariableMayBeConst
        HWND oldWnd = FindWindowW(L"WD_CLASS", NULL);
        if (oldWnd) {
            SendMessageW(oldWnd, WM_CLOSE, 0, 0);
            Sleep(RESTART_DELAY_MS);
        }
        CloseHandle(hMutex);
        hMutex = CreateMutexW(NULL, TRUE, L"WslDock_SingleInstance_Mutex");
        if (!hMutex || GetLastError() == ERROR_ALREADY_EXISTS) {
            if (hMutex) {
                CloseHandle(hMutex);
            }
            return 1;
        }
    }

    initConfigPath();
    loadConfig();
    updateAutostartIfNeeded();
    applyInstanceFromArgs();

    const WNDCLASSEXW WIN_CLASS = {
        sizeof(WNDCLASSEXW), 0, wndProc, 0, 0, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"WD_CLASS", NULL};
    RegisterClassExW(&WIN_CLASS);

    // ReSharper disable once CppLocalVariableMayBeConst
    hwnd = CreateWindowExW(0, L"WD_CLASS", NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

    notifyData.cbSize           = sizeof(NOTIFYICONDATAW);
    notifyData.hWnd             = hwnd;
    notifyData.uID              = ID_TRAY_ICON;
    notifyData.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyData.uCallbackMessage = WM_TRAYICON;
    int initSize                = GetSystemMetrics(SM_CXSMICON);
    int initResId               = resIdForStateAndSize(STATE_INACTIVE, initSize);
    notifyData.hIcon = (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(initResId), IMAGE_ICON, initSize,
                                         initSize, LR_SHARED);
    wcscpy_s(notifyData.szTip, 128, L"WslDock");

    if (!Shell_NotifyIconW(NIM_ADD, &notifyData)) {
        CloseHandle(hMutex);
        return 1;
    }

    applyState(globalState);

    HANDLE hNotify = FindFirstChangeNotificationW(configDir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    if (hNotify == INVALID_HANDLE_VALUE) {
        hNotify = NULL;
    }

    MSG msg;

    while (TRUE) {
        HANDLE waitHandles[3];
        DWORD  handleCount = 0;
        if (hNotify) {
            waitHandles[handleCount++] = hNotify;
        }
        if (hWslProc) {
            waitHandles[handleCount++] = hWslProc;
        }
        if (hSshProc) {
            waitHandles[handleCount++] = hSshProc;
        }

        printf("waiting\n");

        // ReSharper disable once CppLocalVariableMayBeConst
        DWORD dwWait = MsgWaitForMultipleObjects(handleCount, waitHandles, FALSE, INFINITE, QS_ALLINPUT);

        printf("handle: %lu\n", dwWait);

        if (dwWait >= WAIT_OBJECT_0 && dwWait < WAIT_OBJECT_0 + handleCount) {
            DWORD  idx    = dwWait - WAIT_OBJECT_0;
            HANDLE signal = waitHandles[idx];

            if (signal == hNotify) {
                AppState prevState = globalState;
                loadConfig();
                if (globalState != prevState) {
                    AppState loaded = globalState;
                    globalState     = prevState;
                    applyState(loaded);
                }
                updateTray();
                FindNextChangeNotification(hNotify);
            } else if (signal == hWslProc || signal == hSshProc) {
                checkProcesses();
            }
        }

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                goto cleanup;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

cleanup:
    if (hNotify) {
        FindCloseChangeNotification(hNotify);
    }
    CloseHandle(hMutex);
    return 0;
}
