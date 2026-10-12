#include "ui_draw.h"
#include "i18n.h"
#include "title_scroll.h"
#include <algorithm>
#include <cmath>

namespace Ui {
template<class T> static void Release(T*& p) { if (p) { p->Release(); p = nullptr; } }
static D2D1_RECT_F Native(Rect r) { return D2D1::RectF(r.x,r.y,r.x+r.w,r.y+r.h); }
// All canvases share one factory, so Direct2D shares one Direct3D device among
// their hardware DC render targets. A factory per canvas created one device per
// window: about 16 MiB private, 150 handles and 35 driver threads each
// (docs/PERFORMANCE-AUDIT.md). Every Canvas lives on the UI thread.
static ID2D1Factory* sharedFactory = nullptr;
static unsigned sharedFactoryUsers = 0;
Canvas::~Canvas() { Reset(); }
void Canvas::Reset() {
    Release(scrollLayout_);
    DiscardTarget();
    for (auto& p : ellipsis_) Release(p);
    for (auto& p : fonts_) Release(p);
    Release(write_);
    if (factory_) { factory_=nullptr; if (!--sharedFactoryUsers) Release(sharedFactory); }
    themeGeneration_=languageGeneration_=0;
}
bool Canvas::Initialize() {
    if (!factory_) {
        if (!sharedFactory && FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,&sharedFactory))) return false;
        factory_=sharedFactory; ++sharedFactoryUsers;
    }
    if (!write_ && FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,__uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&write_)))) return false;
    if(themeGeneration_!=ThemeGeneration() || languageGeneration_!=I18n::Generation()) {
        Release(scrollLayout_);
        for(auto& p:ellipsis_) Release(p);
        for(auto& p:fonts_) Release(p);
        themeGeneration_=ThemeGeneration();
        languageGeneration_=I18n::Generation();
    }
    const auto& typography=ActiveTheme().typography;
    for (int i=0;i<8;++i) if (!fonts_[i]) {
        int role=i%4;
        const wchar_t* family=role==3?typography.digital:typography.body;
        // Theme families remain intact for Latin/Cyrillic and the digital clock.
        // Native Win7 script families provide reliable CJK/Arabic/Devanagari shaping;
        // DirectWrite's system fallback handles missing glyphs/families.
        if(role!=3 && I18n::ScriptFont()) family=I18n::ScriptFont();
        IDWriteFontCollection* collection=nullptr;
        if(SUCCEEDED(write_->GetSystemFontCollection(&collection,FALSE))) {
            UINT32 index=0;BOOL exists=FALSE;collection->FindFamilyName(family,&index,&exists);
            if(!exists) family=L"Segoe UI";
            collection->Release();
        }
        if (FAILED(write_->CreateTextFormat(family,nullptr,
            static_cast<DWRITE_FONT_WEIGHT>(i>=4?700:typography.weights[role]),
            role==2 && typography.italicHeading?DWRITE_FONT_STYLE_ITALIC:DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,typography.sizes[role],I18n::LocaleName().c_str(),&fonts_[i]))) return false;
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
void Canvas::TextAt(Rect r,const std::wstring& text,uint32_t color,Font font,bool center,bool bold) {
    if (r.w<=0 || r.h<=0) return;
    auto format=fonts_[static_cast<int>(font)+(bold?4:0)];
    const bool arabic=std::any_of(text.begin(),text.end(),[](wchar_t c){
        return (c>=0x0600&&c<=0x08ff)||(c>=0xfb50&&c<=0xfdff)||(c>=0xfe70&&c<=0xfeff);
    });
    // Pure technical values/transport symbols keep their sign/order even in Arabic.
    format->SetReadingDirection(I18n::IsRtl() && arabic && font!=Font::Digital?
        DWRITE_READING_DIRECTION_RIGHT_TO_LEFT:DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
    format->SetTextAlignment(center?DWRITE_TEXT_ALIGNMENT_CENTER:DWRITE_TEXT_ALIGNMENT_LEADING);
    Color(color); target_->DrawText(text.c_str(),static_cast<UINT32>(text.size()),format,Native(r),brush_,D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
void Canvas::TextLayout(float x,float y,IDWriteTextLayout* layout,uint32_t color) {
    Color(color);target_->DrawTextLayout(D2D1::Point2F(x,y),layout,brush_,D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
bool Canvas::ScrollingText(Rect r,const std::wstring& text,double elapsedMs,uint32_t color,Font font) {
    if(r.w<=0 || r.h<=0)return false;
    if(!scrollLayout_ || text!=scrollText_ || r.w!=scrollWidth_ || r.h!=scrollHeight_ || font!=scrollFont_) {
        Release(scrollLayout_);
        auto format=fonts_[static_cast<int>(font)];
        // Metadata is user content. DirectWrite still shapes bidi runs within LTR titles.
        format->SetReadingDirection(DWRITE_READING_DIRECTION_LEFT_TO_RIGHT);
        if(FAILED(write_->CreateTextLayout(text.c_str(),static_cast<UINT32>(text.size()),format,r.w,r.h,&scrollLayout_))) {
            TextAt(r,text,color,font);return false;
        }
        DWRITE_TRIMMING trim{DWRITE_TRIMMING_GRANULARITY_NONE,0,0};
        scrollLayout_->SetTrimming(&trim,nullptr);
        scrollLayout_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        DWRITE_TEXT_METRICS metrics{};
        scrollLayout_->GetMetrics(&metrics);scrollExtent_=metrics.widthIncludingTrailingWhitespace;
        scrollLayout_->SetMaxWidth(std::max(r.w,scrollExtent_+1));
        scrollText_=text;scrollWidth_=r.w;scrollHeight_=r.h;scrollFont_=font;
    }
    float overflow=std::max(0.f,scrollExtent_-r.w);
    Clip(r);Color(color);
    target_->DrawTextLayout(D2D1::Point2F(r.x-TitleScrollOffset(overflow,elapsedMs),r.y),scrollLayout_,brush_);
    Unclip();return overflow>.5f;
}
void Canvas::Button(Rect r,const std::wstring& text,bool active,bool hover,bool pressed,bool enabled,bool focus) {
    Fill(r,!enabled?Surface:active?SelectedButtonFill():pressed?palette.pressed:hover?Hover:Surface,3);
    Stroke(r,focus?Accent:active?Green:Border,3);
    TextAt(r,text,enabled?(active?SelectedButtonText:Text):Muted,Font::Body,true);
}
void Canvas::Slider(Rect r,float value,bool enabled,bool focus) {
    value=std::isfinite(value)?std::max(0.f,std::min(value,1.f)):0;
    float y=r.y+r.h/2, x=r.x+6+(r.w-12)*value;
    Line(r.x+6,y,r.x+r.w-6,y,palette.sliderTrack,4);
    Line(r.x+6,y,x,y,enabled?palette.sliderFill:Muted,4);
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

// A short-lived renderer keeps native popup navigation, submenus and dismissal.
// Resources and item data exist only during TrackPopupMenu's modal loop.
#include <memory>
#include <vector>
namespace Ui {
namespace {
struct MenuEntry { HMENU menu; UINT position,type; std::wstring text; bool submenu; };
struct MenuSkin {
    std::vector<std::unique_ptr<MenuEntry>> entries;
    HFONT font; HBRUSH background; float scale;
    int S(int n) const { return static_cast<int>(std::lround(n*scale)); }
    explicit MenuSkin(float value):scale(value) {
        font=CreateFontW(-S(12),0,0,0,400,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,
            I18n::ScriptFont()?I18n::ScriptFont():ActiveTheme().typography.body);
        background=CreateSolidBrush(NativeColor(Surface));
    }
    void Prepare(HMENU menu) {
        MENUINFO info{};info.cbSize=sizeof(info);info.fMask=MIM_BACKGROUND;info.hbrBack=background;SetMenuInfo(menu,&info);
        for(int i=0;i<GetMenuItemCount(menu);++i) {
            wchar_t text[512]{};MENUITEMINFOW item{};item.cbSize=sizeof(item);
            item.fMask=MIIM_FTYPE|MIIM_STRING|MIIM_SUBMENU;item.dwTypeData=text;item.cch=ARRAYSIZE(text);
            if(!GetMenuItemInfoW(menu,i,TRUE,&item)) continue;
            auto entry=std::make_unique<MenuEntry>(MenuEntry{menu,static_cast<UINT>(i),item.fType,text,item.hSubMenu!=nullptr});
            if(item.hSubMenu) Prepare(item.hSubMenu);
            item.fMask=MIIM_FTYPE|MIIM_DATA;item.fType|=MFT_OWNERDRAW;
            item.dwItemData=reinterpret_cast<ULONG_PTR>(entry.get());SetMenuItemInfoW(menu,i,TRUE,&item);
            entries.push_back(std::move(entry));
        }
    }
    ~MenuSkin() {
        for(auto& e:entries) {
            MENUITEMINFOW item{};item.cbSize=sizeof(item);item.fMask=MIIM_FTYPE|MIIM_DATA;item.fType=e->type;
            SetMenuItemInfoW(e->menu,e->position,TRUE,&item);
            MENUINFO info{};info.cbSize=sizeof(info);info.fMask=MIM_BACKGROUND;info.hbrBack=nullptr;SetMenuInfo(e->menu,&info);
        }
        DeleteObject(font);DeleteObject(background);
    }
    MenuEntry* Find(ULONG_PTR data) {
        for(auto& e:entries) if(reinterpret_cast<ULONG_PTR>(e.get())==data) return e.get();
        return nullptr;
    }
};
MenuSkin* activeMenu=nullptr;
}
BOOL TrackThemedMenu(HMENU menu,UINT flags,int x,int y,HWND owner,float scale) {
    MenuSkin skin(scale);skin.Prepare(menu);auto previous=activeMenu;activeMenu=&skin;
    BOOL result=TrackPopupMenu(menu,flags,x,y,0,owner,nullptr);activeMenu=previous;return result;
}
bool ThemeMenuMessage(UINT message,WPARAM,LPARAM lp,LRESULT& result) {
    if(!activeMenu) return false;
    auto& skin=*activeMenu;
    if(message==WM_MEASUREITEM) {
        auto item=reinterpret_cast<MEASUREITEMSTRUCT*>(lp);
        if(item->CtlType!=ODT_MENU) return false;
        auto entry=skin.Find(item->itemData);if(!entry) return false;
        HDC dc=GetDC(nullptr);auto old=SelectObject(dc,skin.font);SIZE size{};
        GetTextExtentPoint32W(dc,entry->text.c_str(),static_cast<int>(entry->text.size()),&size);
        SelectObject(dc,old);ReleaseDC(nullptr,dc);
        item->itemWidth=size.cx+skin.S(52);item->itemHeight=skin.S(entry->type&MFT_SEPARATOR?10:30);
        result=TRUE;return true;
    }
    if(message!=WM_DRAWITEM) return false;
    auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(item->CtlType!=ODT_MENU) return false;
    auto entry=skin.Find(item->itemData);if(!entry) return false;
    HDC dc=item->hDC;RECT r=item->rcItem;
    bool chosen=(item->itemState&ODS_SELECTED)!=0;
    HBRUSH brush=CreateSolidBrush(NativeColor(chosen?Hover:Surface));FillRect(dc,&r,brush);DeleteObject(brush);
    int saved=SaveDC(dc);SetBkMode(dc,TRANSPARENT);SelectObject(dc,skin.font);
    if(entry->type&MFT_SEPARATOR) {
        HPEN pen=CreatePen(PS_SOLID,1,NativeColor(Border));SelectObject(dc,pen);
        MoveToEx(dc,r.left+skin.S(8),(r.top+r.bottom)/2,nullptr);LineTo(dc,r.right-skin.S(8),(r.top+r.bottom)/2);
        RestoreDC(dc,saved);DeleteObject(pen);
    } else {
        SetTextColor(dc,NativeColor(item->itemState&ODS_DISABLED?Muted:Text));
        RECT text=r;text.left+=skin.S(28);text.right-=skin.S(24);
        DrawTextW(dc,entry->text.c_str(),-1,&text,DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS|
            (I18n::IsRtl()?DT_RTLREADING|DT_RIGHT:0));
        if(item->itemState&ODS_CHECKED) {text=r;text.left+=skin.S(6);text.right=text.left+skin.S(20);DrawTextW(dc,L"✓",1,&text,DT_SINGLELINE|DT_VCENTER);}
        RestoreDC(dc,saved);
    }
    result=TRUE;return true;
}
}
