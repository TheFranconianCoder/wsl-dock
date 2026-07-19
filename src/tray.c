#include "tray.h"

#include <wchar.h>

#include "app_state.h"

// ---------------------------------------------------------------------------
// Icon helpers
// ---------------------------------------------------------------------------

int resIdForStateAndSize(AppState state, int size) {
    // Nächste passende Stufe ermitteln
    int tier; // 0 = 32, 1 = 48, 2 = 64
    if (size <= 32) {
        tier = 0;
    } else if (size <= 48) {
        tier = 1;
    } else {
        tier = 2;
    }

    switch (state) {
    case STATE_ACTIVE:
        return IDI_ACTIVE_32 + tier;
    case STATE_ACTIVE_SSH:
        return IDI_ACTIVE_SSH_32 + tier;
    default:
        return IDI_INACTIVE_32 + tier;
    }
}

static HICON iconForState(AppState state) {
    int size  = GetSystemMetrics(SM_CXSMICON);
    int resId = resIdForStateAndSize(state, size);
    return (HICON)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(resId), IMAGE_ICON, size, size, LR_SHARED);
}

// ---------------------------------------------------------------------------
// Tray update
// ---------------------------------------------------------------------------

void updateTray(void) {
    // ReSharper disable once CppLocalVariableMayBeConst
    HICON hNew = iconForState(globalState);
    if (hNew) {
        notifyData.hIcon = hNew;
        notifyData.uFlags |= NIF_ICON;
        Shell_NotifyIconW(NIM_MODIFY, &notifyData);
    }
    updateTooltip();
}

// ---------------------------------------------------------------------------
// Tooltip
// ---------------------------------------------------------------------------

void updateTooltip(void) {
    wchar_t tooltip[128];

    switch (globalState) {
    case STATE_ACTIVE:
        swprintf_s(tooltip, 128, L"WslDock v%d.%d.%d: %s [active]", APP_VERSION_MAJOR, APP_VERSION_MINOR,
                   APP_VERSION_PATCH, wslInstance);
        break;
    case STATE_ACTIVE_SSH:
        swprintf_s(tooltip, 128, L"WslDock v%d.%d.%d: %s [active + SSH]", APP_VERSION_MAJOR, APP_VERSION_MINOR,
                   APP_VERSION_PATCH, wslInstance);
        break;
    default:
        swprintf_s(tooltip, 128, L"WslDock v%d.%d.%d: [inactive]", APP_VERSION_MAJOR, APP_VERSION_MINOR,
                   APP_VERSION_PATCH);
        break;
    }

    wcscpy_s(notifyData.szTip, 128, tooltip);
    notifyData.uFlags |= NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &notifyData);
}

// ---------------------------------------------------------------------------
// Balloon notification
// ---------------------------------------------------------------------------

void showNotification(const wchar_t* title, const wchar_t* message) {
    notifyData.uFlags |= NIF_INFO;
    wcscpy_s(notifyData.szInfoTitle, 64, title);
    wcscpy_s(notifyData.szInfo, 256, message);
    notifyData.dwInfoFlags = NIIF_INFO;
    notifyData.uTimeout    = 5000;
    Shell_NotifyIconW(NIM_MODIFY, &notifyData);
    notifyData.uFlags &= ~NIF_INFO;
}
