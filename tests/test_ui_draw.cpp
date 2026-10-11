#include "ui_draw.h"
#include "check.h"
static void dc_render_resize_recreate() {
    HDC dc=CreateCompatibleDC(nullptr); CHECK(dc);
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=920; info.bmiHeader.biHeight=-580;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr; HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0); CHECK(bitmap);
    auto old=SelectObject(dc,bitmap);
    {
        Ui::Canvas canvas;
        for (float scale : {1.f,1.25f,1.5f,2.f}) {
            RECT rect{0,0,static_cast<LONG>(460*scale),static_cast<LONG>(290*scale)};
            CHECK(canvas.Begin(dc,rect,scale));
            canvas.Fill({0,0,460,290},Ui::Background);
            canvas.TitleBar(460,L"LiteSic · Áudio",true);
            canvas.Button({8,40,100,28},L"Reproduzir",true,false,false,true,true);
            canvas.Slider({8,80,200,28},0.5f);
            CHECK(canvas.End()); GdiFlush();
            CHECK(GetPixel(dc,static_cast<int>(200*scale),static_cast<int>(200*scale))==RGB(0x3B,0x28,0x20));
            canvas.DiscardTarget(); // Same production recovery as device loss.
        }
        CHECK(!canvas.Begin(dc,{0,0,10,10},0));
    }
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc);
}
int main() { int cases=0; CASE(dc_render_resize_recreate); std::printf("%d UI render cases\n",cases); }
