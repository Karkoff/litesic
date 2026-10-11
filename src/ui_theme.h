#pragma once
#include <cstdint>
#include <cstddef>
#include <windows.h>

// Shared DIP/color contract. No audio or HWND state belongs here.
namespace Ui {
struct ThemePalette {
    uint32_t background, surface, border, bronze, green, accent;
    uint32_t text, muted, danger, display, active, hover;
    uint32_t pressed, selection, sliderTrack, sliderFill, fft, osc, warning;
};
struct Typography {
    const wchar_t* body;
    const wchar_t* digital;
    float sizes[4];
    int weights[4];
    bool italicHeading;
};
struct Theme {
    const wchar_t* id;
    const wchar_t* name;
    const wchar_t* category;
    ThemePalette palette;
    Typography typography;
};
// UI-thread-only live palette. References keep existing rendering call sites simple.
inline ThemePalette palette{0x3B2820,0x2B2018,0x5A4633,0x887B5B,0x00E676,0xC4FF00,
    0xE6E1D7,0xA8A096,0xFF6668,0x10140F,0x23402B,0x513C2C,
    0x5A4633,0x5A4633,0x5A4633,0x00E676,0x00E676,0x00E676,0xFFCC66};
inline const uint32_t &Background=palette.background, &Surface=palette.surface, &Border=palette.border;
inline const uint32_t &Bronze=palette.bronze, &Green=palette.green, &Accent=palette.accent;
inline const uint32_t &Text=palette.text, &Muted=palette.muted, &Danger=palette.danger;
inline const uint32_t &Display=palette.display, &Active=palette.active, &Hover=palette.hover;
constexpr UINT WM_THEME_CHANGED = WM_APP + 88;
size_t ThemeCount();
const Theme& ThemeAt(size_t index);
const Theme& ActiveTheme();
unsigned ThemeGeneration();
// nullptr disables persistence for isolated tests; paths must be absolute.
void ThemeConfigure(const wchar_t* iniPath);
bool SelectTheme(const wchar_t* id, bool save = true);
bool RestoreTheme();
bool ThemeInitializeUser();
struct AppearanceState {
    bool visualizerVisible = true, playlistVisible = true, equalizerVisible = false;
    bool spectrum = false;
    bool playlistFavoritesOnly = false;
    int scalePercent = 0;
    int volumePermille = 800;
};
// Uses the same settings.ini configured for themes; no path means isolated defaults.
AppearanceState LoadAppearance();
bool SaveAppearanceValue(const wchar_t* key, int value);
struct PanelPosition { int left=0,top=0,side=0,anchor=0; };
bool LoadEqualizerPosition(PanelPosition& position);
bool SaveEqualizerPosition(const PanelPosition& position);
double Contrast(uint32_t a, uint32_t b);
inline COLORREF NativeColor(uint32_t rgb) { return RGB((rgb>>16)&255,(rgb>>8)&255,rgb&255); }
void ShowThemes(HWND owner);
bool ThemeDialogMessage(MSG* message);
constexpr float Space = 8, TitleHeight = 32, ControlHeight = 28;
constexpr float PlayerWidth = 460, PlayerHeight = 290;
constexpr float PlaylistWidth = 460, PlaylistHeight = 360;
constexpr float EqWidth = 460, EqHeight = 214;
enum class Font { Body, Small, Heading, Digital };
struct Rect {
    float x, y, w, h;
    bool Contains(float px, float py) const { return px >= x && py >= y && px < x+w && py < y+h; }
};
}
