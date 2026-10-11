#pragma once
#include <windows.h>
// Equalizer controls and attachment to the main player.
void EqWnd_Toggle(HWND main);
void EqWnd_Restore(HWND main);
void EqWnd_ScaleChanged();
void EqWnd_MainMoved();
void EqWnd_Shutdown();
