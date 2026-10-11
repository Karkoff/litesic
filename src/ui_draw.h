#pragma once
#include "ui_theme.h"
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>

namespace Ui {
// Native menu navigation, application palette and typography. The caller owns HMENU.
BOOL TrackThemedMenu(HMENU menu, UINT flags, int x, int y, HWND owner, float scale);
bool ThemeMenuMessage(UINT message, WPARAM wp, LPARAM lp, LRESULT& result);
// UI-thread owned, reusable Direct2D 1.0 / DirectWrite resources per window.
class Canvas {
public:
    Canvas() = default;
    ~Canvas();
    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;
    bool Begin(HDC dc, const RECT& pixels, float scale);
    bool End();
    void DiscardTarget();
    void Fill(Rect r, uint32_t color, float radius = 0);
    void Stroke(Rect r, uint32_t color, float radius = 0, float width = 1);
    void Line(float x1, float y1, float x2, float y2, uint32_t color, float width = 1);
    void Circle(float x, float y, float radius, uint32_t fill, uint32_t border);
    void TextAt(Rect r, const std::wstring& text, uint32_t color = Text,
                Font font = Font::Body, bool center = false);
    void Button(Rect r, const std::wstring& text, bool active = false,
                bool hover = false, bool pressed = false, bool enabled = true, bool focus = false);
    void Slider(Rect r, float value, bool enabled = true, bool focus = false);
    void TitleBar(float width, const std::wstring& caption, bool minimize,
                  int hover = 0, int pressed = 0);
    void Clip(Rect r);
    void Unclip();
private:
    bool Initialize();
    void Color(uint32_t rgb);
    ID2D1Factory* factory_ = nullptr;
    IDWriteFactory* write_ = nullptr;
    ID2D1DCRenderTarget* target_ = nullptr;
    ID2D1SolidColorBrush* brush_ = nullptr;
    IDWriteTextFormat* fonts_[4]{};
    IDWriteInlineObject* ellipsis_[4]{};
    unsigned themeGeneration_ = 0;
};
}
