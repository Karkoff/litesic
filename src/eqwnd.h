#pragma once
#include <windows.h>
// Read-only commissioning panel. No claim of audio processing until M6 DSP.
void EqWnd_Toggle(HWND main);
void EqWnd_Restore(HWND main);
void EqWnd_ScaleChanged();
void EqWnd_Shutdown();
