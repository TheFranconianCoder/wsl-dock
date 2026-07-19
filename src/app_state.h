#pragma once

// ReSharper disable CppLocalVariableMayBeConst
// ReSharper disable CppParameterMayBeConst
// ReSharper disable CppRedundantCastExpression

#include <windows.h>

#include <shellapi.h>

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

enum {
    ID_TRAY_ICON              = 101,
    IDI_INACTIVE_32           = 102,
    IDI_INACTIVE_48           = 103,
    IDI_INACTIVE_64           = 104,
    IDI_ACTIVE_32             = 105,
    IDI_ACTIVE_48             = 106,
    IDI_ACTIVE_64             = 107,
    IDI_ACTIVE_SSH_32         = 108,
    IDI_ACTIVE_SSH_48         = 109,
    IDI_ACTIVE_SSH_64         = 110,
    ID_MENU_SSH               = 201,
    ID_MENU_REBOOT            = 202,
    ID_MENU_EXIT              = 203,
    RESTART_DELAY_MS          = 500,
    CONFIG_RELOAD_DEBOUNCE_MS = 100,
    PROC_TERMINATE_WAIT_MS    = 500,
    WSL_TERMINATE_WAIT_MS     = 5000,

    APP_VERSION_MAJOR = 0,
    APP_VERSION_MINOR = 1,
    APP_VERSION_PATCH = 3,
};

#define WM_TRAYICON (WM_USER + 1)

#define DEFAULT_INSTANCE L"Default"

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

typedef enum { STATE_INACTIVE = 0, STATE_ACTIVE, STATE_ACTIVE_SSH, STATE_COUNT } AppState;

// ---------------------------------------------------------------------------
// Global state  (defined in main.c)
// ---------------------------------------------------------------------------

extern AppState        globalState;
extern wchar_t         wslInstance[128];
extern NOTIFYICONDATAW notifyData;
extern wchar_t         configPath[MAX_PATH];
extern wchar_t         configDir[MAX_PATH];
extern DWORD64         lastConfigLoad;
extern HANDLE          hWslProc;
extern HANDLE          hSshProc;
