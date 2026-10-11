#include "ui_draw.h"
#include <algorithm>
#include <cmath>

namespace Ui {
template<class T> static void Release(T*& p) { if (p) { p->Release(); p = nullptr; } }
static D2D1_RECT_F Native(Rect r) { return D2D1::RectF(r.x,r.y,r.x+r.w,r.y+r.h); }
Canvas::~Canvas() {
    DiscardTarget();
    for (auto& p : ellipsis_) Release(p);
    for (auto& p : fonts_) Release(p);
    Release(write_); Release(factory_);
}
bool Canvas::Initialize() {
    if (!factory_ && FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&factory_))) return false;
    if (!write_ && FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&write_)))) return false;
    const float sizes[] = {12,10,14,27};
    for (int i=0;i<4;++i) if (!fonts_[i]) {
        if (FAILED(write_->CreateTextFormat(i==3?L"Consolas":L"Segoe UI",nullptr,
            i>=2?DWRITE_FONT_WEIGHT_SEMI_BOLD:DWRITE_FONT_WEIGHT_NORMAL,DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,sizes[i],L"pt-BR",&fonts_[i]))) return false;
        fonts_[i]->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        fonts_[i]->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_CHARACTER,0,0};
        if (SUCCEEDED(write_->CreateEllipsisTrimmingSign(fonts_[i],&ellipsis_[i])))
            fonts_[i]->SetTrimming(&trim,ellipsis_[i]);
    }
    if (!target_) {
        auto properties=D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,D2D1_ALPHA_MODE_IGNORE),96,96);
        if (FAILED(factory_->CreateDCRenderTarget(&properties,&target_))) return false;
        if (FAILED(target_->CreateSolidColorBrush(D2D1::ColorF(0),&brush_))) { DiscardTarget(); return false; }
    }
    return true;
}
void Canvas::DiscardTarget() { Release(brush_); Release(target_); }
bool Canvas::Begin(HDC dc, const RECT& pixels, float scale) {
    if (!std::isfinite(scale) || scale<=0 || !Initialize()) return false;
    if (FAILED(target_->BindDC(dc,&pixels))) { DiscardTarget(); return false; }
    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Scale(scale,scale));
    target_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    return true;
}
bool Canvas::End() {
    HRESULT result=target_->EndDraw();
    if (FAILED(result)) DiscardTarget();
    return SUCCEEDED(result);
}
void Canvas::Color(uint32_t rgb) { brush_->SetColor(D2D1::ColorF(rgb)); }
void Canvas::Fill(Rect r,uint32_t color,float radius) {
    Color(color);
    if (radius) target_->FillRoundedRectangle(D2D1::RoundedRect(Native(r),radius,radius),brush_);
    else target_->FillRectangle(Native(r),brush_);
}
void Canvas::Stroke(Rect r,uint32_t color,float radius,float width) {
    Color(color);
    if (radius) target_->DrawRoundedRectangle(D2D1::RoundedRect(Native(r),radius,radius),brush_,width);
    else target_->DrawRectangle(Native(r),brush_,width);
}
void Canvas::Line(float x1,float y1,float x2,float y2,uint32_t color,float width) {
    Color(color); target_->DrawLine(D2D1::Point2F(x1,y1),D2D1::Point2F(x2,y2),brush_,width);
}
void Canvas::Circle(float x,float y,float radius,uint32_t fill,uint32_t border) {
    auto e=D2D1::Ellipse(D2D1::Point2F(x,y),radius,radius);
    Color(fill); target_->FillEllipse(e,brush_); Color(border); target_->DrawEllipse(e,brush_);
}
void Canvas::TextAt(Rect r,const std::wstring& text,uint32_t color,Font font,bool center) {
    if (r.w<=0 || r.h<=0) return;
    auto format=fonts_[static_cast<int>(font)];
    format->SetTextAlignment(center?DWRITE_TEXT_ALIGNMENT_CENTER:DWRITE_TEXT_ALIGNMENT_LEADING);
    Color(color); target_->DrawText(text.c_str(),static_cast<UINT32>(text.size()),format,Native(r),brush_,D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
void Canvas::Button(Rect r,const std::wstring& text,bool active,bool hover,bool pressed,bool enabled,bool focus) {
    Fill(r,pressed?Border:active?Active:hover?Hover:Surface,3);
    Stroke(r,focus?Accent:active?Green:Border,3);
    TextAt(r,text,enabled?(active?Green:Text):Muted,Font::Body,true);
}
void Canvas::Slider(Rect r,float value,bool enabled,bool focus) {
    value=std::isfinite(value)?std::max(0.f,std::min(value,1.f)):0;
    float y=r.y+r.h/2, x=r.x+6+(r.w-12)*value;
    Line(r.x+6,y,r.x+r.w-6,y,Border,4);
    Line(r.x+6,y,x,y,enabled?Green:Muted,4);
    Circle(x,y,5,Bronze,enabled?Text:Muted);
    if (focus) Stroke(r,Accent,3);
}
void Canvas::TitleBar(float width,const std::wstring& caption,bool minimize,int hover,int pressed) {
    Fill({0,0,width,TitleHeight},Border);
    TextAt({12,0,width-84,TitleHeight},caption,Text,Font::Heading);
    Button({width-32,4,28,24},L"×",false,hover==2,pressed==2);
    if(minimize) Button({width-64,4,28,24},L"−",false,hover==1,pressed==1);
}
void Canvas::Clip(Rect r) { target_->PushAxisAlignedClip(Native(r),D2D1_ANTIALIAS_MODE_PER_PRIMITIVE); }
void Canvas::Unclip() { target_->PopAxisAlignedClip(); }
}
