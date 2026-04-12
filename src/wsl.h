#pragma once

#include "app_state.h"

void applyState(AppState newState);
void wslReboot(void);
void checkProcesses(void);
void wslCleanup(void);
