// Real-device check of output recovery (docs/PLAYBACK-IDLE-RECOVERY-2026-10-15.md).
// Uses the default WASAPI endpoint and a silent fixture: nothing is audible.
// It cannot invalidate the endpoint itself; it leaves the device stopped,
// the state a lost endpoint produces, and verifies restart and recreation.
#include "playback.h"
#include <cstdio>
static bool Advances() {
    double before = Audio_GetPositionSeconds(); Sleep(400);
    return Audio_GetPositionSeconds() > before + .2;
}
#define STEP(name, ok) do { bool r = (ok); std::printf("%s %s\n", r ? "PASS" : "FAIL", name); if (!r) failures++; } while (0)
int main() {
    int failures = 0;
    STEP("init default device", Audio_Init()); Audio_SetVolume(1.f);
    Playlist p; p.Add(L"tests\\fixtures\\native-48000.wav");
    PlaybackController c(p, {Audio_Load, Audio_Play, Audio_Stop, Audio_Unload, Audio_SeekSeconds,
                             Audio_GetPositionSeconds, Audio_GetState, Audio_IsAtEnd, Audio_IsActive});
    STEP("play", c.Play() && Advances());
    Audio_Stop(); Audio_TestStopOutput();
    STEP("play after stopped output (restart)", c.Play() && Advances());
    Audio_TestStopOutput();
    STEP("timer check while playing", Audio_CheckOutput() == AUDIO_OUTPUT_OK && Audio_CheckOutput() == AUDIO_OUTPUT_RECOVERED && Advances());
    Audio_TestStopOutput(); // Within 30 s of the last recovery: recreate the engine.
    STEP("play after repeated failure (recreate)", c.Play() && Advances());
    c.Clear(); Audio_Shutdown();
    std::printf("%d failures\n", failures);
    return failures != 0;
}
