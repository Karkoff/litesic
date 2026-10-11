#pragma once
#include "window_snap.h"

// UI-thread authority for the three existing top-level windows. Rendering,
// audio and INI ownership remain in their existing components.
namespace Skin {
enum Panel { Player=0, Playlist=1, Equalizer=2 };
void Bind(Panel panel,HWND window);
void Unbind(Panel panel);
WindowSnap::Dock& Attachment(Panel panel);
int Parent(Panel panel);
void RestoreParent(Panel panel,int parent);
bool ParentRect(Panel panel,RECT& rect);
void BeginMove(Panel panel);
void Moving(Panel panel,RECT& proposed,int tolerance);
void EndMove(Panel panel,int tolerance);
void Constrain(Panel panel,WINDOWPOS& position);
void Changed(Panel panel);
void Reconcile();
void Recover();
bool DockedRect(Panel panel,const RECT& owner,RECT& rect);
}
