#pragma once
#include <windows.h>
constexpr UINT WM_PLAYER_CHANGED = WM_APP + 56;
// A null session path disables persistence (for isolated window tests).
BOOL PlWnd_Create(HINSTANCE instance, HWND main, const wchar_t* sessionFile = nullptr);
void PlWnd_Shutdown();
void PlWnd_Show();
void PlWnd_Toggle();
bool PlWnd_IsVisible();
void PlWnd_Refresh(bool revealCurrent = false);
bool PlWnd_PlaySelected();
void PlWnd_ScaleChanged();
void PlWnd_MainMoved();
void PlWnd_AddFiles(HWND owner, bool playFirst = false);
