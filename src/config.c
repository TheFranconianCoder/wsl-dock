#include "config.h"

#include <shlobj.h>

#include <corecrt.h>
#include <stdio.h>

#include "app_state.h"

// ---------------------------------------------------------------------------
// Config path
// ---------------------------------------------------------------------------

void initConfigPath(void) {
    PWSTR pathLocal;
    if (SHGetKnownFolderPath(&FOLDERID_LocalAppData, 0, NULL, &pathLocal) == S_OK) {
        if (wcslen(pathLocal) + 20 < MAX_PATH) {
            swprintf_s(configDir, MAX_PATH, L"%s\\WslDock", pathLocal);
            CreateDirectoryW(configDir, NULL);
            swprintf_s(configPath, MAX_PATH, L"%s\\wsl_dock.conf", configDir);
        }
        CoTaskMemFree(pathLocal);
    }
}

// ---------------------------------------------------------------------------
// Persist / restore
//
// Format: "<state> <instance>\n"
//   e.g.  "2 Ubuntu-24.04"
//
//   state: 0 = inactive, 1 = active, 2 = active+ssh
// ---------------------------------------------------------------------------

void saveConfig(void) {
    wchar_t tempPath[MAX_PATH];
    swprintf_s(tempPath, MAX_PATH, L"%s.tmp", configPath);

    FILE* configFile = NULL;
    if (_wfopen_s(&configFile, tempPath, L"w") == 0 && configFile) {
        FILE* const SAFE_CONFIG_FILE = configFile;

        fwprintf(SAFE_CONFIG_FILE, L"%d %s", (int)globalState, wslInstance);
        fclose(SAFE_CONFIG_FILE);

        if (!MoveFileExW(tempPath, configPath, MOVEFILE_REPLACE_EXISTING)) {
            FILE* directFile = NULL;
            if (_wfopen_s(&directFile, configPath, L"w") == 0 && directFile) {
                fwprintf(directFile, L"%d %s", (int)globalState, wslInstance);
                fclose(directFile);
            }
            DeleteFileW(tempPath);
        }
    }
}

void loadConfig(void) {
    FILE* configFile = NULL;
    wcscpy_s(wslInstance, 128, DEFAULT_INSTANCE); // immer vorbelegen
    if (_wfopen_s(&configFile, configPath, L"r") == 0 && configFile) {
        int     tempState = 0;
        wchar_t tempInstance[128];
        wcscpy_s(tempInstance, 128, DEFAULT_INSTANCE);
        if (fwscanf_s(configFile, L"%d %127ls", &tempState, tempInstance, (unsigned)_countof(tempInstance)) == 2) {
            if (tempState >= 0 && tempState < STATE_COUNT) {
                globalState = (AppState)tempState;
            }
            if (wcslen(tempInstance) > 0) {
                wcscpy_s(wslInstance, 128, tempInstance);
            }
        }
        fclose(configFile);
    }
}

// ---------------------------------------------------------------------------
// Autostart
// ---------------------------------------------------------------------------

void updateAutostartIfNeeded(void) {
    WCHAR currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ | KEY_WRITE,
                      &hKey) == ERROR_SUCCESS) {
        WCHAR existingPath[MAX_PATH];
        DWORD dataSize = sizeof(existingPath);
        // ReSharper disable once CppLocalVariableMayBeConst
        LSTATUS status = RegQueryValueExW(hKey, L"WslDock", NULL, NULL, (LPBYTE)existingPath, &dataSize);

        if (status != ERROR_SUCCESS || wcscmp(existingPath, currentPath) != 0) {
            RegSetValueExW(hKey, L"WslDock", 0, REG_SZ, (BYTE*)currentPath,
                           (DWORD)((wcslen(currentPath) + 1) * sizeof(WCHAR)));
        }
        RegCloseKey(hKey);
    }
}

// ---------------------------------------------------------------------------
// CLI: WslDock.exe <InstanceName>
// Overrides stored instance name and saves immediately.
// ---------------------------------------------------------------------------

void applyInstanceFromArgs(void) {
    int     argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv && argc >= 2) {
        wcscpy_s(wslInstance, 128, argv[1]);
        saveConfig();
    }
    if (argv) {
        LocalFree((HLOCAL)argv);
    }
}
