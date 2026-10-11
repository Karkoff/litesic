#include "plwnd.h"
#include "mainwnd.h"
#include "playback.h"
#include "playlist_worker.h"
#include "m3u.h"
#include "playlist_view.h"
#include "ui_draw.h"
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shellapi.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr wchar_t kClass[] = L"LiteSic.PlaylistWnd";
enum Command { AddFiles = 501, AddFolder, Remove, OpenList, SaveList, NameAsc,
    NameDesc, DurationAsc, DurationDesc, Mix, Clear, AddMenu, ListMenu };
HWND window = nullptr, mainWindow = nullptr;
HWND search = nullptr;
HFONT searchFont = nullptr;
HBRUSH searchBackground = nullptr;
Ui::Canvas canvas;
PlaylistView view;
std::wstring query;
constexpr int SearchId=520, ClearSearch=521;
PlaylistWorker worker;
std::vector<int> selected;
int anchor = -1, top = 0, insertion = -1, hover = 0, pressed = 0;
bool dragging = false, scrollDrag = false, deferSelection = false;
POINT down{};
float scale = 1;
enum class Dock { None, Bottom, Top, Right, Left };
Dock dock = Dock::None;
int dockOffset = 0;
bool movingList = false;
bool sizeMoveLoop = false;
POINT moveCursor{};
RECT moveOrigin{};
std::wstring status;
std::wstring sessionPath;
int S(int n) { return static_cast<int>(std::lround(n * scale)); }
RECT Client() { RECT r{}; GetClientRect(window, &r); return r; }
RECT WorkArea() {
    MONITORINFO monitor{}; monitor.cbSize=sizeof(monitor);
    if (GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&monitor)) return monitor.rcWork;
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0); return work;
}
// Snap only facing edges with an overlapping span: nearby windows cannot
// become attached through a distant corner or while rows are reordered.
Dock Snap(RECT& list) {
    RECT parent{}; GetWindowRect(mainWindow,&parent);
    int tolerance=S(12), best=tolerance+1, dx=0, dy=0;
    Dock side=Dock::None;
    auto choose=[&](Dock candidate,int distance,int x,int y) {
        if (std::abs(distance)<best) { best=std::abs(distance); side=candidate; dx=x; dy=y; }
    };
    if (list.left<parent.right && list.right>parent.left) {
        choose(Dock::Bottom,list.top-parent.bottom,0,parent.bottom-list.top);
        choose(Dock::Top,list.bottom-parent.top,0,parent.top-list.bottom);
    }
    if (list.top<parent.bottom && list.bottom>parent.top) {
        choose(Dock::Right,list.left-parent.right,parent.right-list.left,0);
        choose(Dock::Left,list.right-parent.left,parent.left-list.right,0);
    }
    if (side!=Dock::None) OffsetRect(&list,dx,dy);
    return side;
}
void RememberDock(Dock side,const RECT& list) {
    RECT parent{}; GetWindowRect(mainWindow,&parent);
    dock=side;
    dockOffset=(side==Dock::Bottom || side==Dock::Top) ? list.left-parent.left : list.top-parent.top;
}
void AvoidOverlap() {
    RECT parent{}, list{}, overlap{};
    GetWindowRect(mainWindow,&parent); GetWindowRect(window,&list);
    if (!IntersectRect(&overlap,&parent,&list)) return;
    RECT work=WorkArea(); int width=list.right-list.left,height=list.bottom-list.top;
    int x=std::max<int>(work.left,std::min<int>(list.left,work.right-width));
    int y=parent.bottom+S(4);
    if(y+height>work.bottom) y=std::max<int>(work.top,parent.top-height-S(4));
    SetWindowPos(window,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
RECT List() { RECT r = Client(); r.left = S(8); r.top = S(124); r.right -= S(20); r.bottom -= S(28); return r; }
int Visible() { RECT r = List(); return std::max<int>(1, (r.bottom - r.top) / S(24)); }
void ClampTop() { top = std::max(0, std::min(top, view.Count() - Visible())); }
void LayoutSearch() {
    if(!search) return;
    RECT r=Client();
    MoveWindow(search,S(16),S(80),std::max<int>(S(80),r.right-S(56)),S(24),TRUE);
    if(searchFont) DeleteObject(searchFont);
    searchFont=CreateFontW(-S(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    SendMessageW(search,WM_SETFONT,reinterpret_cast<WPARAM>(searchFont),TRUE);
}
void Fill(HDC dc, RECT r, COLORREF color) { HBRUSH brush = CreateSolidBrush(color); FillRect(dc, &r, brush); DeleteObject(brush); }
void Text(HDC dc, RECT r, const std::wstring& text, COLORREF color, UINT flags = DT_LEFT) {
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &r,
        flags | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
}
RECT Button(int id) {
    RECT r = Client();
    if (id == -1) return {r.right - S(32), S(4), r.right - S(4), S(28)};
    if (id == ClearSearch) return {r.right-S(36),S(78),r.right-S(8),S(106)};
    int width = (r.right - S(40)) / 4;
    int column=id==AddFiles?0:id==AddFolder?1:id==Remove?2:3;
    int x = S(8) + column * (width + S(8));
    return {x, S(40), x + width, S(68)};
}
int HitButton(POINT p) {
    for (int id : {-1, static_cast<int>(AddFiles),static_cast<int>(AddFolder), static_cast<int>(Remove), static_cast<int>(ListMenu),ClearSearch}) {
        RECT r = Button(id); if (PtInRect(&r, p)) return id;
    }
    return 0;
}
int HitRow(POINT p) {
    RECT r = List();
    if (!PtInRect(&r, p)) return -1;
    int row = top + (p.y - r.top) / S(24);
    return view.Canonical(row);
}
std::vector<uint64_t> SelectedIds() {
    std::vector<uint64_t> ids;
    for (int index : selected) if (auto track = PlayerPlaylist().At(index)) ids.push_back(track->id);
    return ids;
}
void RestoreSelection(const std::vector<uint64_t>& ids) {
    selected.clear();
    for (auto id : ids) { int index = PlayerPlaylist().Find(id); if (index >= 0) selected.push_back(index); }
    std::sort(selected.begin(), selected.end());
    anchor = selected.empty() ? -1 : selected.front();
}
void Error(const std::wstring& text) { status = text; PlWnd_Refresh(); }
void AddTrack(const Track& incoming) {
    if (PlayerPlaylist().Count() >= 100000) { status = L"Limite de 100.000 faixas atingido."; return; }
    uint64_t id = PlayerPlaylist().Add(FullPath(incoming.path));
    Track* track = PlayerPlaylist().At(PlayerPlaylist().Count() - 1);
    if (!incoming.displayTitle.empty()) track->displayTitle = incoming.displayTitle;
    track->duration = incoming.duration;
    PlaylistJob job; job.id = id; job.path = track->path;
    if (!worker.Submit(std::move(job))) {
        track->metadataPending = false; status = L"Leitura de metadados indisponível.";
    }
}
void AddPath(const std::wstring& path) {
    status.clear();
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        PlaylistJob job; job.kind = PlaylistJob::Scan; job.path = FullPath(path);
        if (!worker.Submit(std::move(job))) Error(L"Não foi possível iniciar a varredura.");
    } else if (IsSupportedAudioFile(path.c_str())) {
        Track track; track.path = path; AddTrack(track);
    }
}
void FolderDialog() {
    BROWSEINFOW info{}; info.hwndOwner = window;
    info.lpszTitle = L"Adicionar pasta de áudio (inclui subpastas)";
    info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_EDITBOX;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&info);
    if (pidl) {
        wchar_t path[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, path)) AddPath(path);
        CoTaskMemFree(pidl);
    }
    PlWnd_Refresh();
}
void ListDialog(bool save) {
    wchar_t path[32768] = {};
    OPENFILENAMEW info{}; info.lStructSize = sizeof(info); info.hwndOwner = window;
    info.lpstrFile = path; info.nMaxFile = ARRAYSIZE(path);
    info.lpstrFilter = L"Listas M3U UTF-8\0*.m3u;*.m3u8\0\0";
    info.lpstrDefExt = L"m3u8"; info.lpstrTitle = save ? L"Salvar lista M3U" : L"Abrir lista M3U";
    info.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&info) : GetOpenFileNameW(&info))) return;
    PlaylistJob job; job.kind = save ? PlaylistJob::Save : PlaylistJob::Open; job.path = path;
    if (save) job.tracks = PlayerPlaylist().Tracks();
    // Opening appends; a failed open never destroys the current playlist.
    if (!worker.Submit(std::move(job))) Error(L"Não foi possível iniciar a operação M3U.");
    else { status = save ? L"Salvando lista..." : L"Abrindo lista..."; PlWnd_Refresh(); }
}
void RemoveSelected() {
    for (auto it = selected.rbegin(); it != selected.rend(); ++it) PlayerPlayback().RemoveAt(*it);
    selected.clear(); anchor = -1; status.clear(); ClampTop(); PlWnd_Refresh();
}
void Menu(bool add, POINT point) {
    HMENU menu = CreatePopupMenu();
    if (add) {
        AppendMenuW(menu, MF_STRING, AddFiles, L"Arquivos...");
        AppendMenuW(menu, MF_STRING, AddFolder, L"Pasta e subpastas...");
    } else {
        AppendMenuW(menu, MF_STRING, OpenList, L"Abrir M3U (adicionar)...");
        AppendMenuW(menu, MF_STRING, SaveList, L"Salvar M3U...");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, NameAsc, L"Nome: A → Z");
        AppendMenuW(menu, MF_STRING, NameDesc, L"Nome: Z → A");
        AppendMenuW(menu, MF_STRING, DurationAsc, L"Duração: menor → maior");
        AppendMenuW(menu, MF_STRING, DurationDesc, L"Duração: maior → menor");
        AppendMenuW(menu, MF_STRING, Mix, L"Embaralhar ordem visível");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, Clear, L"Limpar lista");
    }
    ClientToScreen(window, &point);
    TrackPopupMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, 0, window, nullptr);
    DestroyMenu(menu);
}
void Paint(HWND hwnd) {
    PAINTSTRUCT ps{}; HDC target = BeginPaint(hwnd, &ps); RECT r = Client();
    if(canvas.Begin(target,r,scale)) {
        using namespace Ui;
        const float width=r.right/scale,height=r.bottom/scale;
        canvas.Fill({0,0,width,height},Background);
        canvas.TitleBar(width,L"LITESIC · PLAYLIST",false,hover==-1?2:0,pressed==-1?2:0);
        for(int id : {static_cast<int>(AddFiles),static_cast<int>(AddFolder),static_cast<int>(Remove),static_cast<int>(ListMenu)}) {
            RECT b=Button(id);
            canvas.Button({b.left/scale,b.top/scale,(b.right-b.left)/scale,(b.bottom-b.top)/scale},
                id==AddFiles?L"+ Arquivos":id==AddFolder?L"+ Pasta":id==Remove?L"Remover":L"Lista ▾",
                false,hover==id,pressed==id,id!=Remove || !selected.empty());
        }
        canvas.Fill({8,76,width-16,32},Surface,3); canvas.Stroke({8,76,width-16,32},Border,3);
        if(query.empty() && GetFocus()!=search) canvas.TextAt({16,78,width-60,28},L"Pesquisar músicas...",Muted);
        canvas.Button({width-36,78,28,28},L"×",false,hover==ClearSearch,pressed==ClearSearch);
        canvas.Fill({8,112,width-28,12},Surface);
        canvas.TextAt({12,110,24,14},L"#",Bronze,Font::Small);
        canvas.TextAt({42,110,width-130,14},L"Título",Muted,Font::Small);
        canvas.TextAt({width-80,110,56,14},L"Duração",Muted,Font::Small,true);
        RECT list=List();
        Rect area{list.left/scale,list.top/scale,(list.right-list.left)/scale,(list.bottom-list.top)/scale};
        canvas.Fill(area,Surface); canvas.Clip(area);
        for(int row=top;row<view.Count() && row<top+Visible()+1;++row) {
            int index=view.Canonical(row); const Track& track=*PlayerPlaylist().At(index);
            float y=area.y+(row-top)*24;
            bool current=index==PlayerPlaylist().GetCurrentIndex();
            bool chosen=std::binary_search(selected.begin(),selected.end(),index);
            if(chosen || current) canvas.Fill({area.x,y,area.w,24},chosen?Border:Active);
            canvas.Line(area.x,y+24,area.x+area.w,y+24,Background);
            canvas.TextAt({12,y,28,24},current?L"▶":std::to_wstring(index+1),current?Green:Muted,Font::Small);
            canvas.TextAt({42,y,width-126,24},track.displayTitle,track.failed?Danger:track.metadataPending?Muted:Ui::Text);
            wchar_t duration[32]=L"--:--";
            if(track.duration>=0) { unsigned seconds=static_cast<unsigned>(std::min(track.duration,359999.));
                swprintf(duration,ARRAYSIZE(duration),L"%u:%02u",seconds/60,seconds%60); }
            canvas.TextAt({width-80,y,56,24},duration,Muted,Font::Small,true);
        }
        if(!view.Count()) canvas.TextAt(area,query.empty()?L"Adicione arquivos ou uma pasta":L"Nenhuma música encontrada",Muted,Font::Body,true);
        if(dragging && insertion>=0) canvas.Line(area.x,area.y+(insertion-top)*24,area.x+area.w,area.y+(insertion-top)*24,Green,2);
        canvas.Unclip();
        canvas.Fill({width-16,area.y,8,area.h},Surface,3);
        if(view.Count()>Visible()) {
            float thumb=std::max(16.f,area.h*Visible()/view.Count());
            canvas.Fill({width-16,area.y+(area.h-thumb)*top/(view.Count()-Visible()),8,thumb},Bronze,3);
        }
        std::wstring footer=status;
        if(footer.empty()) {
            footer=std::to_wstring(PlayerPlaylist().Count())+L" faixas · "+std::to_wstring(selected.size())+L" selecionadas";
            if(!query.empty()) footer+=L" · "+std::to_wstring(view.Count())+L" visíveis";
            if(worker.Busy()) footer+=L" · lendo...";
            else if(PlayerPlaylist().Count()) {
                double seconds=0;bool known=true;
                for(const auto& track:PlayerPlaylist().Tracks()) {if(track.duration<0) known=false;else seconds+=track.duration;}
                if(known) { wchar_t duration[32]{};unsigned total=static_cast<unsigned>(std::min(seconds,359999.));
                    swprintf(duration,ARRAYSIZE(duration),L" · %u:%02u",total/60,total%60);footer+=duration; }
            }
        }
        canvas.TextAt({8,height-26,width-16,24},footer,Muted,Font::Small);
        if(canvas.End()) { EndPaint(hwnd,&ps); return; }
    }
    HDC dc = CreateCompatibleDC(target); HBITMAP bitmap = CreateCompatibleBitmap(target, r.right, r.bottom);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontW(-S(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Tahoma");
    HGDIOBJ oldFont = SelectObject(dc, font); SetBkMode(dc, TRANSPARENT);
    Fill(dc, r, RGB(60,40,30)); Fill(dc, {0,0,r.right,S(24)}, RGB(90,66,50));
    Text(dc, {S(6),0,r.right-S(25),S(24)}, L"LITESIC · LISTA", RGB(240,216,144));
    for (int id : {-1, static_cast<int>(AddMenu), static_cast<int>(Remove), static_cast<int>(ListMenu)}) {
        RECT b = Button(id);
        Fill(dc, b, pressed == id ? RGB(160,128,64) : hover == id ? RGB(138,122,90) : RGB(106,90,58));
        Text(dc, b, id == -1 ? L"×" : id == AddMenu ? L"Adicionar" : id == Remove ? L"Remover" : L"Lista",
            id == -1 ? RGB(255,64,64) : RGB(240,216,144), DT_CENTER);
    }
    RECT list = List(); Fill(dc, list, RGB(18,18,18));
    int saved = SaveDC(dc); IntersectClipRect(dc, list.left, list.top, list.right, list.bottom);
    for (int index = top; index < PlayerPlaylist().Count(); ++index) {
        RECT row = {list.left, list.top + (index-top)*S(18), list.right, list.top + (index-top+1)*S(18)};
        if (row.top >= list.bottom) break;
        const Track& track = *PlayerPlaylist().At(index);
        bool isSelected = std::binary_search(selected.begin(), selected.end(), index);
        if (isSelected) Fill(dc, row, RGB(90,74,42));
        std::wstring title = (index == PlayerPlaylist().GetCurrentIndex() ? L"> " : L"  ") + track.displayTitle;
        RECT name = row; name.left += S(3); name.right -= S(49);
        Text(dc, name, title, track.failed ? RGB(255,128,112) : track.metadataPending ? RGB(184,160,120) : RGB(240,216,144));
        RECT duration = row; duration.left = duration.right - S(47); duration.right -= S(3);
        wchar_t time[32] = L"--:--";
        if (track.duration >= 0) {
            unsigned seconds = static_cast<unsigned>(std::min(track.duration, 359999.0));
            swprintf(time, ARRAYSIZE(time), L"%u:%02u", seconds/60, seconds%60);
        }
        Text(dc, duration, time, RGB(184,160,120), DT_RIGHT);
    }
    if (!PlayerPlaylist().Count()) Text(dc, list, L"Adicione arquivos ou arraste uma pasta", RGB(184,160,120), DT_CENTER);
    if (dragging && insertion >= 0) {
        int y = list.top + (insertion-top)*S(18);
        Fill(dc, {list.left,y-S(1),list.right,y+S(1)}, RGB(102,204,68));
    }
    RestoreDC(dc, saved);
    RECT rail = {list.right+S(3),list.top,r.right-S(4),list.bottom}; Fill(dc,rail,RGB(42,42,42));
    int count = PlayerPlaylist().Count();
    if (count > Visible()) {
        int height = rail.bottom - rail.top, thumb = std::max(S(12), height*Visible()/count);
        int y = rail.top + (height-thumb)*top/(count-Visible());
        Fill(dc, {rail.left,y,rail.right,y+thumb}, RGB(192,160,96));
    }
    std::wstring footer = status;
    if (footer.empty()) footer = std::to_wstring(count) + L" faixas · " + std::to_wstring(selected.size()) +
        L" selecionadas" + (worker.Busy() ? L" · lendo..." : L"");
    Text(dc, {S(5),r.bottom-S(23),r.right-S(5),r.bottom-S(3)}, footer, RGB(184,160,120));
    BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,oldFont); DeleteObject(font); SelectObject(dc,oldBitmap); DeleteObject(bitmap); DeleteDC(dc);
    EndPaint(hwnd,&ps);
}
LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    switch (message) {
    case WM_CREATE: window = hwnd; scale = MainWnd_DPIScaleFloat(1); DragAcceptFiles(hwnd,TRUE);
        query.clear(); view.Update(PlayerPlaylist(),query);
        searchBackground=CreateSolidBrush(RGB(0x2B,0x20,0x18));
        search=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,
            0,0,1,1,hwnd,reinterpret_cast<HMENU>(SearchId),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(search,EM_SETLIMITTEXT,512,0); LayoutSearch();
        SendMessageW(search,EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(L"Pesquisar músicas..."));
        if (!worker.Start(hwnd)) status = L"Falha ao iniciar leitura em segundo plano.";
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_SIZE: LayoutSearch(); ClampTop(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_CTLCOLOREDIT: SetTextColor(reinterpret_cast<HDC>(wp),RGB(0xE6,0xE1,0xD7));
        SetBkColor(reinterpret_cast<HDC>(wp),RGB(0x2B,0x20,0x18)); return reinterpret_cast<LRESULT>(searchBackground);
    case WM_ENTERSIZEMOVE:
        sizeMoveLoop=true; GetCursorPos(&moveCursor); GetWindowRect(hwnd,&moveOrigin); return 0;
    case WM_MOVING: {
        movingList=true; dock=Dock::None;
        auto proposed=reinterpret_cast<RECT*>(lp);
        POINT cursor{}; GetCursorPos(&cursor);
        // Derive mouse movement from the drag origin, not the last snapped
        // position, so small consecutive movements can escape the magnet.
        // Keyboard moves retain the native proposed rectangle.
        if (sizeMoveLoop && (cursor.x!=moveCursor.x || cursor.y!=moveCursor.y)) {
            int x=moveOrigin.left+cursor.x-moveCursor.x,y=moveOrigin.top+cursor.y-moveCursor.y;
            OffsetRect(proposed,x-proposed->left,y-proposed->top);
        }
        Snap(*proposed);
        return TRUE;
    }
    case WM_EXITSIZEMOVE: {
        sizeMoveLoop=false;
        if (movingList) {
            RECT list{}; GetWindowRect(hwnd,&list);
            RememberDock(Snap(list),list);
            SetWindowPos(hwnd,nullptr,list.left,list.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            movingList=false;
        }
        // Resizing an attached playlist preserves its facing edge.
        PlWnd_MainMoved();
        return 0;
    }
    case WM_GETMINMAXINFO: {
        auto info = reinterpret_cast<MINMAXINFO*>(lp);
        info->ptMinTrackSize = {S(275),S(180)}; return 0;
    }
    case WM_DPICHANGED: {
        scale = LOWORD(wp) / 96.0f * MainWnd_UserScaleMultiplier();
        auto suggested = reinterpret_cast<RECT*>(lp);
        SetWindowPos(hwnd,nullptr,suggested->left,suggested->top,suggested->right-suggested->left,
            suggested->bottom-suggested->top,SWP_NOZORDER|SWP_NOACTIVATE);
        PlWnd_MainMoved(); return 0;
    }
    case WM_NCHITTEST: {
        ScreenToClient(hwnd,&point); RECT r = Client(); int edge = std::max(4,S(4));
        bool left=point.x<edge,right=point.x>=r.right-edge,up=point.y<edge,bottom=point.y>=r.bottom-edge;
        if (up && left) return HTTOPLEFT;
        if (up && right) return HTTOPRIGHT;
        if (bottom && left) return HTBOTTOMLEFT;
        if (bottom && right) return HTBOTTOMRIGHT;
        if (left) return HTLEFT;
        if (right) return HTRIGHT;
        if (up) return HTTOP;
        if (bottom) return HTBOTTOM;
        if (point.y<S(32) && !HitButton(point)) return HTCAPTION;
        return HTCLIENT;
    }
    case WM_LBUTTONDOWN: {
        SetFocus(hwnd); pressed = HitButton(point);
        if (pressed) { SetCapture(hwnd); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
        if(point.y>=S(76) && point.y<S(108)) { ShowWindow(search,SW_SHOW); SetFocus(search); return 0; }
        RECT list=List();
        if (point.x>list.right && point.y>=list.top && point.y<list.bottom && view.Count()>Visible()) {
            scrollDrag=true; SetCapture(hwnd);
            top=(point.y-list.top)*(view.Count()-Visible())/std::max<int>(1,list.bottom-list.top);
            ClampTop(); PlWnd_Refresh(); return 0;
        }
        int row=HitRow(point); if (row<0) return 0;
        status.clear();
        bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        deferSelection = !ctrl && !shift && std::binary_search(selected.begin(),selected.end(),row);
        if (!deferSelection) {
            if(query.empty()) selected=SelectRows(selected,PlayerPlaylist().Count(),row,anchor,ctrl,shift);
            else {
                std::vector<int> visibleSelected;
                for(int index:selected) if(view.Row(index)>=0) visibleSelected.push_back(view.Row(index));
                auto rows=SelectRows(visibleSelected,view.Count(),view.Row(row),view.Row(anchor),ctrl,shift);
                selected.clear(); for(int index:rows) selected.push_back(view.Canonical(index));
                std::sort(selected.begin(),selected.end());
            }
        }
        if (!shift) anchor=row;
        down=point; dragging=false; insertion=-1; SetCapture(hwnd); PlWnd_Refresh(); return 0;
    }
    case WM_MOUSEMOVE: {
        int newHover=HitButton(point); if (newHover!=hover) { hover=newHover; InvalidateRect(hwnd,nullptr,FALSE); }
        if (GetCapture()!=hwnd || pressed) return 0;
        RECT list=List();
        if (scrollDrag) {
            top=(point.y-list.top)*(view.Count()-Visible())/std::max<int>(1,list.bottom-list.top);
            ClampTop(); PlWnd_Refresh(); return 0;
        }
        if (query.empty() && !selected.empty() && (std::abs(point.x-down.x)>=S(4) || std::abs(point.y-down.y)>=S(4))) dragging=true;
        if (dragging) {
            if (point.y<list.top+S(10)) --top; else if (point.y>list.bottom-S(10)) ++top;
            ClampTop(); insertion=std::max(0,std::min<int>(PlayerPlaylist().Count(),top+(point.y-list.top+S(12))/S(24)));
            PlWnd_Refresh();
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        bool moved=dragging, scrolled=scrollDrag; int button=pressed;
        dragging=false; scrollDrag=false; pressed=0;
        if (GetCapture()==hwnd) ReleaseCapture();
        if (button && HitButton(point)==button) {
            if (button==-1) ShowWindow(hwnd,SW_HIDE);
            else if (button==Remove) RemoveSelected();
            else if (button==AddFiles) PlWnd_AddFiles(hwnd);
            else if (button==AddFolder) FolderDialog();
            else if (button==ClearSearch) { SetWindowTextW(search,L""); SetFocus(search); }
            else Menu(button==AddMenu,{Button(button).left,Button(button).bottom});
        } else if (moved && insertion>=0) {
            auto ids=SelectedIds(); PlayerPlaylist().MoveSelection(selected,insertion); RestoreSelection(ids);
        } else if (!scrolled && deferSelection) { int row=HitRow(point); if (row>=0) selected={row}; }
        insertion=-1; deferSelection=false; ClampTop(); PlWnd_Refresh(); return 0;
    }
    case WM_CAPTURECHANGED: dragging=false; scrollDrag=false; pressed=0; InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDBLCLK: {
        int row=HitRow(point);
        if (row>=0 && !PlayerPlayback().PlayIndex(row)) Error(L"Não foi possível tocar a faixa selecionada.");
        PlWnd_Refresh(true); return 0;
    }
    case WM_MOUSEWHEEL: top-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*3; ClampTop(); PlWnd_Refresh(); return 0;
    case WM_KEYDOWN:
        if(wp==VK_TAB) { ShowWindow(search,SW_SHOW); SetFocus(search); }
        else if (wp==VK_DELETE) RemoveSelected();
        else if (wp=='A' && (GetKeyState(VK_CONTROL)&0x8000)) {
            status.clear();
            selected=view.Indices(); PlWnd_Refresh();
        } else if (wp==VK_RETURN && !selected.empty()) { PlayerPlayback().PlayIndex(selected.front()); PlWnd_Refresh(true); }
        else if((wp==VK_UP || wp==VK_DOWN || wp==VK_HOME || wp==VK_END) && view.Count()) {
            int row=view.Row(selected.empty()?-1:selected.back());
            row=wp==VK_HOME?0:wp==VK_END?view.Count()-1:std::clamp(row+(wp==VK_UP?-1:1),0,view.Count()-1);
            selected={view.Canonical(row)};anchor=selected.front();
            if(row<top) top=row;else if(row>=top+Visible()) top=row-Visible()+1;
            PlWnd_Refresh();
        }
        return 0;
    case WM_CONTEXTMENU: ScreenToClient(hwnd,&point); Menu(false,point); return 0;
    case WM_COMMAND: {
        if(LOWORD(wp)==SearchId && HIWORD(wp)==EN_KILLFOCUS && query.empty()) {
            ShowWindow(search,SW_HIDE); InvalidateRect(hwnd,nullptr,FALSE); return 0;
        }
        if(LOWORD(wp)==SearchId && HIWORD(wp)==EN_CHANGE) {
            wchar_t buffer[513]{}; GetWindowTextW(search,buffer,ARRAYSIZE(buffer));
            query=buffer; top=0; status.clear();
            ShowWindow(search,query.empty() && GetFocus()!=search?SW_HIDE:SW_SHOW);
            PlWnd_Refresh(); return 0;
        }
        auto ids=SelectedIds();
        switch(LOWORD(wp)) {
        case AddFiles: PlWnd_AddFiles(hwnd); break;
        case AddFolder: FolderDialog(); break;
        case OpenList: ListDialog(false); break;
        case SaveList: ListDialog(true); break;
        case Remove: RemoveSelected(); break;
        case NameAsc: PlayerPlaylist().Sort(false,true); break;
        case NameDesc: PlayerPlaylist().Sort(false,false); break;
        case DurationAsc: PlayerPlaylist().Sort(true,true); break;
        case DurationDesc: PlayerPlaylist().Sort(true,false); break;
        case Mix: PlayerPlaylist().ShuffleVisible(); break;
        case Clear: worker.NewGeneration(); PlayerPlayback().Clear(); selected.clear(); anchor=-1; top=0; status.clear(); break;
        }
        if (LOWORD(wp)>=NameAsc && LOWORD(wp)<=Mix) RestoreSelection(ids);
        PlWnd_Refresh(); return 0;
    }
    case WM_DROPFILES: {
        HDROP drop=reinterpret_cast<HDROP>(wp);
        UINT count=DragQueryFileW(drop,0xffffffff,nullptr,0);
        for(UINT i=0;i<count;++i) {
            UINT length=DragQueryFileW(drop,i,nullptr,0); std::vector<wchar_t> path(length+1);
            DragQueryFileW(drop,i,path.data(),length+1); AddPath(path.data());
        }
        DragFinish(drop); PlWnd_Refresh(); return 0;
    }
    case WM_PLAYLIST_RESULTS:
        for(auto& result:worker.Drain()) {
            if (result.generation!=worker.Generation()) continue;
            if (!result.error.empty()) { status=result.error; continue; }
            if (result.kind==PlaylistJob::Metadata) {
                if (auto track=PlayerPlaylist().At(PlayerPlaylist().Find(result.id))) {
                    track->metadataPending=false; track->metadataLoaded=true; track->failed=!result.metadata.readable;
                    if (result.metadata.duration>=0) track->duration=result.metadata.duration;
                    if (!result.metadata.title.empty()) track->displayTitle=result.metadata.artist.empty() ? result.metadata.title :
                        result.metadata.artist+L" — "+result.metadata.title;
                }
            } else if (result.kind==PlaylistJob::Save) status=L"Lista salva.";
            else {
                if (result.kind==PlaylistJob::Open) status=L"Lista adicionada.";
                for(const auto& track:result.tracks) AddTrack(track);
            }
        }
        ClampTop(); PlWnd_Refresh(); return 0;
    case WM_CLOSE: ShowWindow(hwnd,SW_HIDE); return 0;
    case WM_DESTROY:
        worker.Stop();
        if (!sessionPath.empty()) {
            std::wstring error;
            if (!SaveM3U(sessionPath,PlayerPlaylist().Tracks(),error))
                MessageBoxW(mainWindow,(L"Não foi possível preservar a playlist.\n"+error).c_str(),L"LiteSic",MB_OK|MB_ICONERROR);
        }
        sessionPath.clear(); search=nullptr; DeleteObject(searchFont); searchFont=nullptr;
        DeleteObject(searchBackground); searchBackground=nullptr; canvas.DiscardTarget(); window=nullptr; return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}

BOOL PlWnd_Create(HINSTANCE instance, HWND main, const wchar_t* sessionFile) {
    mainWindow=main;
    sessionPath=sessionFile?sessionFile:L"";
    selected.clear(); anchor=-1; top=0; status.clear();
    movingList=false; sizeMoveLoop=false; dock=Dock::None;
    WNDCLASSEXW cls{}; cls.cbSize=sizeof(cls); cls.style=CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
    cls.lpfnWndProc=Proc; cls.hInstance=instance; cls.hCursor=LoadCursorW(nullptr,IDC_ARROW); cls.lpszClassName=kClass;
    if (!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return FALSE;
    scale=MainWnd_DPIScaleFloat(1);
    RECT parent{}; GetWindowRect(main,&parent); RECT work=WorkArea();
    int width=S(460),height=S(360),x=std::max(work.left,std::min(parent.left,work.right-width));
    int y=parent.bottom;
    Dock initial=Dock::Bottom;
    if (y+height>work.bottom) {
        if (parent.top-height>=work.top) { y=parent.top-height; initial=Dock::Top; }
        else if (parent.right+width<=work.right) { x=parent.right; y=parent.top; initial=Dock::Right; }
        else if (parent.left-width>=work.left) { x=parent.left-width; y=parent.top; initial=Dock::Left; }
        else {
            // If neither side fits, place the pair from the top of the work
            // area, reducing the initial playlist height down to its minimum.
            height=std::max(S(180),std::min<int>(height,work.bottom-work.top-(parent.bottom-parent.top)));
            int mainY=std::max<int>(work.top,work.bottom-height-(parent.bottom-parent.top));
            SetWindowPos(main,nullptr,parent.left,mainY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
            GetWindowRect(main,&parent); y=parent.bottom;
        }
    }
    window=CreateWindowExW(WS_EX_ACCEPTFILES,kClass,L"LiteSic — Lista de reprodução",WS_POPUP,
        x,y,width,height,main,nullptr,instance,nullptr);
    if (window) {
        if (!sessionPath.empty()) {
            DWORD attributes=GetFileAttributesW(sessionPath.c_str());
            if (attributes!=INVALID_FILE_ATTRIBUTES || GetLastError()!=ERROR_FILE_NOT_FOUND) {
                std::vector<Track> tracks; std::wstring error;
                if (LoadM3U(sessionPath,tracks,error)) {
                    for (const auto& track:tracks) AddTrack(track);
                } else {
                    status=error;
                    // Do not replace an unreadable session with an empty list.
                    sessionPath.clear();
                }
            }
        }
        PlWnd_Refresh();
        RECT list{}; GetWindowRect(window,&list); RememberDock(initial,list);
        PlWnd_MainMoved(); ShowWindow(window,SW_SHOWNOACTIVATE); UpdateWindow(window);
    }
    return window!=nullptr;
}
void PlWnd_Shutdown() { if (window) DestroyWindow(window); mainWindow=nullptr; dock=Dock::None; }
void PlWnd_MainMoved() {
    if (!window || !mainWindow || dock==Dock::None || movingList || IsIconic(mainWindow)) return;
    RECT parent{},list{}; GetWindowRect(mainWindow,&parent); GetWindowRect(window,&list);
    int x=parent.left+dockOffset,y=parent.top+dockOffset;
    switch (dock) {
    case Dock::Bottom: y=parent.bottom; break;
    case Dock::Top: y=parent.top-(list.bottom-list.top); break;
    case Dock::Right: x=parent.right; break;
    case Dock::Left: x=parent.left-(list.right-list.left); break;
    case Dock::None: return;
    }
    if (x!=list.left || y!=list.top)
        SetWindowPos(window,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
void PlWnd_Show() { if(window) { ShowWindow(window,SW_SHOW); SetForegroundWindow(window); } }
void PlWnd_Toggle() { if(window) { ShowWindow(window,IsWindowVisible(window)?SW_HIDE:SW_SHOWNOACTIVATE); PlWnd_Refresh(); } }
bool PlWnd_IsVisible() { return window && IsWindowVisible(window); }
void PlWnd_Refresh(bool revealCurrent) {
    int current=PlayerPlaylist().GetCurrentIndex();
    view.Update(PlayerPlaylist(),query);
    if (revealCurrent && current>=0) {
        if (view.Row(current)<0 && search) SetWindowTextW(search,L"");
        selected={current}; anchor=current;
        top=view.Row(current)-Visible()/2;
    }
    view.Update(PlayerPlaylist(),query); ClampTop();
    selected.erase(std::remove_if(selected.begin(),selected.end(),[](int index) { return view.Row(index)<0; }),selected.end());
    if(view.Row(anchor)<0) anchor=selected.empty()?-1:selected.front();
    if(window) InvalidateRect(window,nullptr,FALSE);
    if(mainWindow) PostMessageW(mainWindow,WM_PLAYER_CHANGED,0,0);
}
bool PlWnd_PlaySelected() {
    int index=selected.empty()?-1:selected.front();
    bool ok=index>=0 && index!=PlayerPlaylist().GetCurrentIndex()
        ? PlayerPlayback().PlayIndex(index) : PlayerPlayback().Play();
    PlWnd_Refresh(true);
    return ok;
}
void PlWnd_ScaleChanged() {
    if(!window) return;
    float old=scale; scale=MainWnd_DPIScaleFloat(1); RECT r=Client();
    SetWindowPos(window,nullptr,0,0,static_cast<int>(r.right/old*scale),static_cast<int>(r.bottom/old*scale),
        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    if (dock!=Dock::None) PlWnd_MainMoved(); else AvoidOverlap();
    ClampTop(); PlWnd_Refresh();
}
void PlWnd_AddFiles(HWND owner, bool playFirst) {
    std::vector<wchar_t> buffer(32768);
    OPENFILENAMEW info{}; info.lStructSize=sizeof(info); info.hwndOwner=owner;
    info.lpstrFile=buffer.data(); info.nMaxFile=static_cast<DWORD>(buffer.size());
    info.lpstrFilter=L"Áudio\0*.mp3;*.wav;*.flac;*.ogg\0\0";
    info.Flags=OFN_EXPLORER|OFN_ALLOWMULTISELECT|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&info)) {
        if (CommDlgExtendedError()==FNERR_BUFFERTOOSMALL) Error(L"Seleção muito grande; adicione uma pasta.");
        return;
    }
    int first=PlayerPlaylist().Count();
    std::wstring base=buffer.data(); const wchar_t* next=buffer.data()+base.size()+1;
    if (!*next) AddPath(base);
    else while(*next) { AddPath(base+L"\\"+next); next+=wcslen(next)+1; }
    status.clear();
    if (playFirst && first<PlayerPlaylist().Count() && !PlayerPlayback().PlayIndex(first)) Error(L"Não foi possível tocar o arquivo.");
    PlWnd_Refresh();
}
