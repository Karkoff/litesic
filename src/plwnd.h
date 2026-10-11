#pragma once
#include <windows.h>
constexpr UINT WM_PLAYER_CHANGED = WM_APP + 56;
BOOL PlWnd_Create(HINSTANCE instance, HWND main);
void PlWnd_Shutdown();
void PlWnd_Show();
void PlWnd_Toggle();
bool PlWnd_IsVisible();
void PlWnd_Refresh();
void PlWnd_ScaleChanged();
void PlWnd_MainMoved();
void PlWnd_AddFiles(HWND owner, bool playFirst = false);
