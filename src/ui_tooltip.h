#pragma once
#include "ui_draw.h"
#include <functional>
namespace Ui::Tooltips {
struct Target { int id=0; RECT rect{}; };
using Hit = std::function<Target(POINT)>;
using Describe = std::function<std::wstring(int)>;
using Scale = std::function<float()>;
// UI thread only. Regions are resolved from current layout, never cached across resize.
bool Attach(HWND surface, Hit hit, Describe describe, Scale scale);
bool AttachControl(HWND control, const char* key, Scale scale);
void Hide(HWND surface);
void Refresh();
Target Resolve(HWND surface, POINT point);
std::wstring Description(HWND surface,int id);
HWND Popup(HWND surface);
size_t AttachedCount();
RECT Place(RECT anchor,SIZE size,RECT work,int gap);
}
