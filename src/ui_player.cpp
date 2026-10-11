#include "ui_player.h"
#include <cmath>

int PlayerLayoutHeight(bool visible) { return visible ? MAINWND_HEIGHT : MAINWND_HEIGHT-(VIZ_H+8); }
Ui::Rect PlayerControlRect(int id,bool visible) {
    const float footerY=FOOTER_BUTTON_Y-(visible?0:VIZ_H+8);
    switch(id) {
    case 1:return {BTN_PREV_X,NAV_Y,NAV_BTN_W,NAV_BTN_H};
    case 2:return {BTN_NEXT_X,NAV_Y,NAV_BTN_W,NAV_BTN_H};
    case 3:return {BTN_SHUFFLE_X,TOGGLE_Y,TOGGLE_W,TOGGLE_H};
    case 4:return {BTN_REPEAT_X,TOGGLE_Y,TOGGLE_W,TOGGLE_H};
    case 5:return {BTN_PLAY_X,NAV_Y,NAV_BTN_W,NAV_BTN_H};
    case 6:return {BTN_PAUSE_X,NAV_Y,NAV_BTN_W,NAV_BTN_H};
    case 7:return {BTN_STOP_X,NAV_Y,NAV_BTN_W,NAV_BTN_H};
    case 8:return {SEEK_X,SEEK_Y,SEEK_W,SEEK_H};
    case 9:return {VOLUME_SLIDER_X,VOLPAN_Y,VOLUME_SLIDER_W,VOLPAN_H};
    case 10:return {PAN_SLIDER_X,VOLPAN_Y,PAN_SLIDER_W,VOLPAN_H};
    case 11:return {BTN_LIST_X,footerY,FOOTER_BUTTON_W,FOOTER_BUTTON_H};
    case 12:return {BTN_EQ_X,footerY,FOOTER_BUTTON_W,FOOTER_BUTTON_H};
    case 13:return visible?Ui::Rect{364,VIZ_Y,40,20}:Ui::Rect{};
    case 14:return visible?Ui::Rect{408,VIZ_Y,40,20}:Ui::Rect{};
    case 16:return {276,footerY,FOOTER_BUTTON_W,FOOTER_BUTTON_H};
    default:return {0,0,0,0};
    }
}
int PlayerControlAt(float x,float y,bool visible) {
    for(int i=1;i<=16;++i) if(PlayerControlRect(i,visible).Contains(x,y)) return i;
    return 0;
}
static void Triangle(Ui::Canvas& c,float x,float y,float direction,uint32_t color) {
    c.Line(x-4*direction,y-6,x+5*direction,y,color,2);
    c.Line(x+5*direction,y,x-4*direction,y+6,color,2);
    c.Line(x-4*direction,y+6,x-4*direction,y-6,color,2);
}
static void ModeIcon(Ui::Canvas& c,int id,float x,float y,uint32_t color,bool one) {
    if(id==3) {
        c.Line(x-9,y-6,x-5,y-6,color,2);
        c.Line(x-5,y-6,x+5,y+6,color,2);
        c.Line(x+5,y+6,x+10,y+6,color,2);
        c.Line(x-9,y+6,x-5,y+6,color,2);
        c.Line(x-5,y+6,x+5,y-6,color,2);
        c.Line(x+5,y-6,x+10,y-6,color,2);
        for(float dy : {-6.f,6.f}) {
            c.Line(x+6,y+dy-3,x+10,y+dy,color,2);
            c.Line(x+10,y+dy,x+6,y+dy+3,color,2);
        }
    } else {
        c.Line(x-8,y-6,x+9,y-6,color,2);
        c.Line(x+9,y-6,x+9,y+2,color,2);
        c.Line(x+8,y+6,x-9,y+6,color,2);
        c.Line(x-9,y+6,x-9,y-2,color,2);
        c.Line(x+5,y-10,x+9,y-6,color,2);
        c.Line(x+9,y-6,x+5,y-2,color,2);
        c.Line(x-5,y+2,x-9,y+6,color,2);
        c.Line(x-9,y+6,x-5,y+10,color,2);
        if(one) c.TextAt({x-4,y-7,8,14},L"1",color,Ui::Font::Small,true);
    }
}
bool PaintPlayer(Ui::Canvas& c,HDC dc,const RECT& pixels,float scale,const PlayerViewState& s) {
    using namespace Ui;
    if(!c.Begin(dc,pixels,scale)) return false;
    const float height=static_cast<float>(PlayerLayoutHeight(s.visualizerVisible));
    c.Fill({0,0,PlayerWidth,height},Background);
    c.Stroke({.5f,.5f,PlayerWidth-1,height-1},Border,3);
    c.TitleBar(PlayerWidth,L"♫  LITESIC",true,s.titleHover,s.titlePressed);
    c.Fill({8,38,444,50},Display,3);
    c.Stroke({LCD_X,LCD_Y,LCD_W,LCD_H},Active,2);
    c.TextAt({LCD_X,LCD_Y,LCD_W,LCD_H},s.time,Green,Font::Digital,true);
    c.TextAt({TITLE_X,TITLE_Y,TITLE_W,23},s.title,Green,Font::Heading);
    c.TextAt({TITLE_X,TITLE_Y+23,TITLE_W,19},s.subtitle,Muted,Font::Small);
    c.TextAt({12,SEEK_Y,34,SEEK_H},L"TEMPO",Muted,Font::Small);
    c.Slider(PlayerControlRect(8),s.seek,s.loaded,s.focus==8);
    c.TextAt({412,SEEK_Y,40,SEEK_H},s.duration,Text,Font::Small,true);
    c.Line(12,116,448,116,Border);
    for(int id : {3,1,5,6,7,2,4}) {
        auto r=PlayerControlRect(id); float x=r.x+r.w/2,y=r.y+r.h/2;
        bool enabled=id==3 || id==4 || s.loaded || ((id==1 || id==2 || id==5) && s.hasPlaylist);
        bool active=(id==3 && s.shuffle) || (id==4 && s.repeat!=RepeatMode::Off) ||
            (id==5 && s.playback==AUDIO_STATE_PLAYING) || (id==6 && s.playback==AUDIO_STATE_PAUSED);
        c.Circle(x,y,17,s.pressed==id?Border:s.hover==id?Hover:Surface,
            s.focus==id?Accent:active?Green:Bronze);
        uint32_t color=enabled?(active?Green:Text):Muted;
        if(id==3 || id==4) ModeIcon(c,id,x,y,Green,s.repeat==RepeatMode::One);
        if(id==5) Triangle(c,x,y,1,color);
        if(id==6) {c.Line(x-3,y-6,x-3,y+6,color,3);c.Line(x+3,y-6,x+3,y+6,color,3);}
        if(id==7) c.Fill({x-5,y-5,10,10},color,1);
        if(id==1 || id==2) {Triangle(c,x,y,id==1?-1:1,color);c.Line(x+(id==1?-8:8),y-6,x+(id==1?-8:8),y+6,color,2);}
    }
    c.Line(12,160,448,160,Border);
    c.TextAt({12,VOLPAN_Y,52,VOLPAN_H},L"Volume",Text,Font::Small);
    c.Slider(PlayerControlRect(9),s.volume,true,s.focus==9);
    c.TextAt({202,VOLPAN_Y,40,VOLPAN_H},std::to_wstring(static_cast<int>(std::lround(s.volume*100)))+L"%",Text,Font::Small);
    c.TextAt({250,VOLPAN_Y,55,VOLPAN_H},L"Balanço",Text,Font::Small);
    c.TextAt({306,VOLPAN_Y,12,VOLPAN_H},L"L",Muted,Font::Small);
    c.Slider(PlayerControlRect(10),s.pan,true,s.focus==10);
    c.TextAt({440,VOLPAN_Y,12,VOLPAN_H},L"R",Muted,Font::Small);
    if(s.visualizerVisible) {
        DrawVisualizerD2D(c,{VIZ_X,VIZ_Y,VIZ_W,VIZ_H},s.visualization);
        c.Button(PlayerControlRect(13),L"FFT",s.visualization==VIZ_MODE_SPECTRUM,s.hover==13,false,true,s.focus==13);
        c.Button(PlayerControlRect(14),L"OSC",s.visualization==VIZ_MODE_OSCILLOSCOPE,s.hover==14,false,true,s.focus==14);
    }
    c.TextAt({12,height-24,256,22},s.technical,Muted,Font::Small);
    for(int id : {16,11,12}) {
        auto r=PlayerControlRect(id,s.visualizerVisible);
        bool active=(id==11 && s.playlistVisible) || (id==16 && s.visualizerVisible);
        c.Fill(r,s.pressed==id?Border:s.hover==id?Hover:active?Active:Surface,3);
        c.Stroke(r,s.focus==id?Accent:active?Green:Bronze,3);
        c.TextAt(r,id==16?L"VIZ":id==11?L"LISTA":L"EQ …",id==12?Muted:Text,Font::Small,true);
    }
    return c.End();
}
