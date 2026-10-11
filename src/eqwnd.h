#pragma once
#include <windows.h>
// Equalizer controls and attachment to the main player.
void EqWnd_Toggle(HWND main);
bool EqWnd_IsVisible();
void EqWnd_Restore(HWND main);
void EqWnd_ScaleChanged();
void EqWnd_MainMoved(bool preserveDock = false);
bool EqWnd_DockedRect(const RECT& owner, RECT& panel);
void EqWnd_Shutdown();
