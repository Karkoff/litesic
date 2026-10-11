#pragma once
#include <cstdint>

// Shared DIP/color contract. No audio or HWND state belongs here.
namespace Ui {
constexpr uint32_t Background = 0x3B2820, Surface = 0x2B2018, Border = 0x5A4633;
constexpr uint32_t Bronze = 0x887B5B, Green = 0x00E676, Accent = 0xC4FF00;
constexpr uint32_t Text = 0xE6E1D7, Muted = 0xA8A096, Danger = 0xFF6668;
constexpr uint32_t Display = 0x10140F, Active = 0x23402B, Hover = 0x513C2C;
constexpr float Space = 8, TitleHeight = 32, ControlHeight = 28;
constexpr float PlayerWidth = 460, PlayerHeight = 290;
constexpr float PlaylistWidth = 460, PlaylistHeight = 360;
constexpr float EqWidth = 460, EqHeight = 210;
enum class Font { Body, Small, Heading, Digital };
struct Rect {
    float x, y, w, h;
    bool Contains(float px, float py) const { return px >= x && py >= y && px < x+w && py < y+h; }
};
}
