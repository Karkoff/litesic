#pragma once
#include "ui_draw.h"
#include "mainwnd.h"
#include "audio.h"
#include "playback.h"
#include "visualizer.h"

struct PlayerViewState {
    std::wstring title, subtitle, technical, time, duration;
    bool loaded = false, shuffle = false, playlistVisible = false, hasPlaylist = false;
    bool visualizerVisible = true;
    AudioState playback = AUDIO_STATE_STOPPED;
    RepeatMode repeat = RepeatMode::Off;
    VizMode visualization = VIZ_MODE_OSCILLOSCOPE;
    float seek = 0, volume = .8f, pan = .5f;
    double titleElapsedMs = 0;
    int hover = 0, pressed = 0, focus = 0;
    TitleBarButton titleHover = BTN_NONE, titlePressed = BTN_NONE;
};
bool PaintPlayer(Ui::Canvas& canvas,HDC dc,const RECT& pixels,float scale,const PlayerViewState& state,bool* titleOverflow = nullptr);
int PlayerLayoutHeight(bool visualizerVisible);
Ui::Rect PlayerControlRect(int id, bool visualizerVisible = true);
int PlayerControlAt(float x,float y, bool visualizerVisible = true);
