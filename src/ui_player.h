#pragma once
#include "ui_draw.h"
#include "mainwnd.h"
#include "audio.h"
#include "playback.h"
#include "visualizer.h"

struct PlayerViewState {
    std::wstring title, subtitle, technical, time, duration;
    bool loaded = false, shuffle = false, playlistVisible = false, hasPlaylist = false;
    AudioState playback = AUDIO_STATE_STOPPED;
    RepeatMode repeat = RepeatMode::Off;
    VizMode visualization = VIZ_MODE_OSCILLOSCOPE;
    float seek = 0, volume = .8f, pan = .5f;
    int hover = 0, pressed = 0, focus = 0;
    TitleBarButton titleHover = BTN_NONE, titlePressed = BTN_NONE;
};
bool PaintPlayer(Ui::Canvas& canvas,HDC dc,const RECT& pixels,float scale,const PlayerViewState& state);
Ui::Rect PlayerControlRect(int id);
int PlayerControlAt(float x,float y);
