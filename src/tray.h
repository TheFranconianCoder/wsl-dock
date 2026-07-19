#pragma once

#include "app_state.h"

int  resIdForStateAndSize(AppState state, int size);
void updateTray(void);
void updateTooltip(void);
void showNotification(const wchar_t* title, const wchar_t* message);
