#include "plwnd.h"
#include "window_snap.h"
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
#include <cwctype>

namespace {
constexpr wchar_t kClass[] = L"LiteSic.PlaylistWnd";
enum Command { AddFiles = 501, AddFolder, Remove, OpenList, SaveList, NameAsc,
    NameDesc, DurationAsc, DurationDesc, Mix, Clear, AddMenu, ListMenu, TrackInfo=PL_COMMAND_INFORMATION };
HWND window = nullptr, mainWindow = nullptr;
HWND information=nullptr,informationEdit=nullptr;
HFONT informationFont=nullptr;
HBRUSH informationBackground=nullptr;
Track informationTrack;
std::wstring informationText;
HWND search = nullptr;
HFONT searchFont = nullptr;
HBRUSH searchBackground = nullptr;
Ui::Canvas canvas;
PlaylistView view;
std::wstring query;
constexpr int SearchId=520, ClearSearch=521, GeneralTab=522, FavoritesTab=523;
constexpr UINT_PTR ResultsTimer=524;
bool resultsScheduled=false;
bool favoritesOnly=false,starClick=false;
uint64_t contextId=0;
PlaylistWorker worker;
std::vector<int> selected;
int anchor = -1, top = 0, insertion = -1, hover = 0, pressed = 0;
bool dragging = false, scrollDrag = false, deferSelection = false;
POINT down{};
float scale = 1;
using Dock = WindowSnap::Side;
Dock dock = Dock::None;
WindowSnap::Anchor dockAnchor = WindowSnap::Anchor::Start;
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
Dock Snap(RECT& list) {
    RECT parent{}; GetWindowRect(mainWindow,&parent);
    auto attachment=WindowSnap::Snap(list,parent,S(24));
    dockAnchor=attachment.anchor;
    return attachment.side;
}
void RememberDock(Dock side,const RECT&) { dock=side; }
bool RestoreGeometry(RECT& panel,Dock& side) {
    if(sessionPath.empty())return false;
    const auto ini=sessionPath+L".ini";
    auto read=[&](const wchar_t* key,int fallback){return static_cast<int>(GetPrivateProfileIntW(L"Geometry",key,fallback,ini.c_str()));};
    if(read(L"Version",0)!=1)return false;
    int w=read(L"Width",0),h=read(L"Height",0),d=read(L"Dock",-1),a=read(L"Anchor",-1);
    int x=read(L"Left",100001),y=read(L"Top",100001),mx=read(L"MainLeft",100001),my=read(L"MainTop",100001);
    if(w<275||w>4096||h<180||h>4096||d<0||d>4||a<0||a>2||
       x< -100000||x>100000||y< -100000||y>100000||mx< -100000||mx>100000||my< -100000||my>100000)return false;
    RECT owner{};GetWindowRect(mainWindow,&owner);
    owner={mx,my,mx+owner.right-owner.left,my+owner.bottom-owner.top};
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    if(!GetMonitorInfoW(MonitorFromRect(&owner,MONITOR_DEFAULTTONEAREST),&monitor))return false;
    WindowSnap::KeepVisible(owner,monitor.rcWork);
    panel={x,y,x+S(w),y+S(h)};side=static_cast<Dock>(d);dockAnchor=static_cast<WindowSnap::Anchor>(a);
    if(side!=Dock::None){
        WindowSnap::Place(panel,owner,{side,dockAnchor});
        RECT group{},fitted{};UnionRect(&group,&owner,&panel);fitted=group;
        WindowSnap::KeepVisible(fitted,monitor.rcWork);
        OffsetRect(&owner,fitted.left-group.left,fitted.top-group.top);
        OffsetRect(&panel,fitted.left-group.left,fitted.top-group.top);
    }else{
        if(GetMonitorInfoW(MonitorFromRect(&panel,MONITOR_DEFAULTTONEAREST),&monitor))
            WindowSnap::KeepVisible(panel,monitor.rcWork);
    }
    SetWindowPos(mainWindow,nullptr,owner.left,owner.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    return true;
}
bool SaveGeometry(const std::wstring& ini){
    RECT panel{},owner{};GetWindowRect(window,&panel);GetWindowRect(mainWindow,&owner);
    if(IsIconic(mainWindow)){WINDOWPLACEMENT placement{};placement.length=sizeof(placement);
        if(GetWindowPlacement(mainWindow,&placement))owner=placement.rcNormalPosition;}
    std::wstring state;
    auto add=[&](const wchar_t* key,int value){state+=std::wstring(key)+L"="+std::to_wstring(value)+L'\0';};
    add(L"Version",1);add(L"Left",panel.left);add(L"Top",panel.top);
    add(L"Width",static_cast<int>(std::lround((panel.right-panel.left)/scale)));
    add(L"Height",static_cast<int>(std::lround((panel.bottom-panel.top)/scale)));
    add(L"Dock",static_cast<int>(dock));add(L"Anchor",static_cast<int>(dockAnchor));
    add(L"MainLeft",owner.left);add(L"MainTop",owner.top);state+=L'\0';
    return WritePrivateProfileSectionW(L"Geometry",state.c_str(),ini.c_str())!=FALSE;
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
RECT List() { RECT r = Client(); r.left = S(8); r.top = S(84); r.right -= S(20); r.bottom -= S(28); return r; }
int Visible() { RECT r = List(); return std::max<int>(1, (r.bottom - r.top) / S(24)); }
void ClampTop() { top = std::max(0, std::min(top, view.Count() - Visible())); }
void LayoutSearch() {
    if(!search) return;
    RECT r=Client();
    MoveWindow(search,S(176),S(40),std::max<int>(S(24),r.right-S(216)),S(24),TRUE);
    if(searchFont) DeleteObject(searchFont);
    searchFont=CreateFontW(-S(12),0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,Ui::ActiveTheme().typography.body);
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
    if (id == GeneralTab)return {S(8),S(38),S(68),S(66)};
    if (id == FavoritesTab)return {S(76),S(38),S(164),S(66)};
    if (id == ClearSearch) return {r.right-S(36),S(38),r.right-S(8),S(66)};
    int width = S(FOOTER_BUTTON_W);
    int column=id==AddMenu?0:id==Remove?1:2;
    int x = r.right-S(8)-(3-column)*width-(2-column)*S(8);
    return {x, r.bottom-S(22), x + width, r.bottom-S(22)+S(FOOTER_BUTTON_H)};
}
// A small outlined eraser and cleared baseline; independent of icon fonts.
template<class Line> void EraserIcon(float x,float y,Line line) {
    line(x-6,y+1,x+1,y-6);line(x+1,y-6,x+7,y);
    line(x+7,y,x,y+7);line(x,y+7,x-6,y+1);
    line(x-3,y-2,x+3,y+4);line(x-7,y+8,x+7,y+8);
}
int HitButton(POINT p) {
    for (int id : {-1, static_cast<int>(AddMenu), static_cast<int>(Remove), static_cast<int>(ListMenu),ClearSearch,GeneralTab,FavoritesTab}) {
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
    track->favorite = incoming.favorite;
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
void Menu(bool add, POINT point, bool upwards=false, int trackIndex=-1) {
    HMENU menu = CreatePopupMenu();
    if (add) {
        AppendMenuW(menu, MF_STRING, AddFiles, L"Arquivos...");
        AppendMenuW(menu, MF_STRING, AddFolder, L"Pasta e subpastas...");
    } else {
        if(auto track=PlayerPlaylist().At(trackIndex)){contextId=track->id;AppendMenuW(menu,MF_STRING,TrackInfo,L"Informações da música...");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);}
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
    Ui::TrackThemedMenu(menu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | (upwards?TPM_BOTTOMALIGN:0), point.x, point.y, window, scale);
    DestroyMenu(menu);
}
void Star(Ui::Canvas& target,float x,float y,bool favorite){
    target.TextAt({x-12,y-12,24,24},favorite?L"★":L"☆",favorite?Ui::Green:Ui::Muted,Ui::Font::Body,true);
}
std::wstring InformationText(const Track& track){
    const auto& m=track.metadata;
    auto value=[](const std::wstring& s){return s.empty()?std::wstring(L"Não informado"):s;};
    std::wstring format=track.path.substr(track.path.find_last_of(L'.')+1);
    for(auto& c:format)c=std::towupper(c);
    std::wstring result=L"Título: "+value(m.title.empty()?track.displayTitle:m.title)+
        L"\r\nArtista: "+value(m.artist)+L"\r\nÁlbum: "+value(m.album)+L"\r\nGênero: "+value(m.genre)+
        L"\r\nAno/data: "+value(m.year)+L"\r\nFaixa: "+value(m.trackNumber)+L"\r\nFormato: "+format;
    wchar_t duration[48]=L"Não informada";
    if(track.duration>=0){unsigned seconds=static_cast<unsigned>(std::min(track.duration,359999.));swprintf(duration,48,L"%u:%02u",seconds/60,seconds%60);}
    result+=L"\r\nDuração: "+std::wstring(duration)+L"\r\nAmostragem: "+(m.sampleRate?std::to_wstring(m.sampleRate)+L" Hz":L"Não informada")+
        L"\r\nCanais: "+(m.channels?std::to_wstring(m.channels):L"Não informados")+
        L"\r\nTamanho: "+std::to_wstring(m.fileBytes)+L" bytes";
    if(m.fileBytes&&track.duration>0)result+=L"\r\nTaxa média aproximada (arquivo): "+std::to_wstring(static_cast<unsigned long long>(m.fileBytes*8./track.duration/1000))+L" kbps";
    result+=L"\r\nFavorita: "+std::wstring(track.favorite?L"Sim":L"Não")+L"\r\nArquivo: "+track.path;
    if(track.metadataPending)result+=L"\r\n\r\nLendo metadados...";
    else if(track.failed)result+=L"\r\n\r\nNão foi possível ler o arquivo de áudio.";
    return result;
}
void RefreshInformation(){
    if(!informationEdit)return;
    if(auto track=PlayerPlaylist().At(PlayerPlaylist().Find(informationTrack.id)))informationTrack=*track;
    auto text=InformationText(informationTrack);
    if(text!=informationText){informationText=std::move(text);SetWindowTextW(informationEdit,informationText.c_str());}
}
LRESULT CALLBACK InformationEditProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(msg==WM_KEYDOWN&&wp==VK_ESCAPE){PostMessageW(GetParent(hwnd),WM_CLOSE,0,0);return 0;}
    if(msg==WM_NCDESTROY)RemoveWindowSubclass(hwnd,InformationEditProc,1);
    return DefSubclassProc(hwnd,msg,wp,lp);
}
LRESULT CALLBACK InformationProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:
        information=hwnd;informationText.clear();
        informationEdit=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL,
            S(12),S(12),S(536),S(328),hwnd,reinterpret_cast<HMENU>(801),GetModuleHandleW(nullptr),nullptr);
        SetWindowSubclass(informationEdit,InformationEditProc,1,0);
        CreateWindowExW(0,L"BUTTON",L"Fechar",WS_CHILD|WS_VISIBLE|WS_TABSTOP,S(448),S(348),S(100),S(28),hwnd,reinterpret_cast<HMENU>(802),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(hwnd,Ui::WM_THEME_CHANGED,0,0);RefreshInformation();return 0;
    case Ui::WM_THEME_CHANGED:{
        HFONT old=informationFont;informationFont=CreateFontW(-S(12),0,0,0,400,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,Ui::ActiveTheme().typography.body);
        SendMessageW(informationEdit,WM_SETFONT,reinterpret_cast<WPARAM>(informationFont),TRUE);
        SendDlgItemMessageW(hwnd,802,WM_SETFONT,reinterpret_cast<WPARAM>(informationFont),TRUE);if(old)DeleteObject(old);
        if(informationBackground)DeleteObject(informationBackground);
        informationBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        InvalidateRect(hwnd,nullptr,TRUE);InvalidateRect(informationEdit,nullptr,TRUE);return 0;}
    case WM_SIZE:{RECT r{};GetClientRect(hwnd,&r);MoveWindow(informationEdit,S(12),S(12),std::max<int>(1,r.right-S(24)),std::max<int>(1,r.bottom-S(60)),TRUE);
        MoveWindow(GetDlgItem(hwnd,802),r.right-S(112),r.bottom-S(40),S(100),S(28),TRUE);return 0;}
    case WM_CTLCOLORSTATIC:case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Text));SetBkColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Surface));return reinterpret_cast<LRESULT>(informationBackground);
    case WM_ERASEBKGND:{RECT r{};GetClientRect(hwnd,&r);FillRect(reinterpret_cast<HDC>(wp),&r,informationBackground);return 1;}
    case WM_COMMAND:if(LOWORD(wp)==802)DestroyWindow(hwnd);return 0;
    case WM_CLOSE:DestroyWindow(hwnd);return 0;
    case WM_DESTROY:information=informationEdit=nullptr;DeleteObject(informationFont);DeleteObject(informationBackground);informationFont=nullptr;informationBackground=nullptr;return 0;
    }return DefWindowProcW(hwnd,msg,wp,lp);
}
void ShowInformation(const Track& track){
    informationTrack=track;
    if(information){RefreshInformation();ShowWindow(information,SW_SHOW);SetForegroundWindow(information);return;}
    WNDCLASSW cls{};cls.lpfnWndProc=InformationProc;cls.hInstance=GetModuleHandleW(nullptr);cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.lpszClassName=L"LiteSic.TrackInfoWnd";
    if(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)return;
    RECT parent{};GetWindowRect(window,&parent);RECT r{0,0,S(560),S(388)};
    DWORD style=WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN;AdjustWindowRectEx(&r,style,FALSE,WS_EX_TOOLWINDOW);
    HWND h=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,L"Informações da música",style,parent.left,parent.top,r.right-r.left,r.bottom-r.top,window,nullptr,cls.hInstance,nullptr);
    if(h){ShowWindow(h,SW_SHOWNOACTIVATE);SetFocus(informationEdit);}
}
void Paint(HWND hwnd) {
    PAINTSTRUCT ps{}; HDC target = BeginPaint(hwnd, &ps); RECT r = Client();
    if(canvas.Begin(target,r,scale)) {
        using namespace Ui;
        const float width=r.right/scale,height=r.bottom/scale;
        canvas.Fill({0,0,width,height},Background);
        canvas.TitleBar(width,L"PLAYLIST",false,hover==-1?2:0,pressed==-1?2:0);
        for(int id : {static_cast<int>(AddMenu),static_cast<int>(Remove),static_cast<int>(ListMenu)}) {
            RECT b=Button(id);
            Rect button{b.left/scale,b.top/scale,(b.right-b.left)/scale,(b.bottom-b.top)/scale};
            canvas.Fill(button,pressed==id?Border:hover==id?Hover:Surface,3);
            canvas.Stroke(button,Bronze,3);
            canvas.TextAt(button,id==AddMenu?L"ADIC ▾":id==Remove?L"REMOVER":L"LISTA ▾",
                id==Remove && selected.empty()?Muted:Ui::Text,Font::Small,true);
        }
        for(int id:{GeneralTab,FavoritesTab}){RECT b=Button(id);canvas.Button({b.left/scale,b.top/scale,(b.right-b.left)/scale,(b.bottom-b.top)/scale},id==GeneralTab?L"Geral":L"Favoritas",favoritesOnly==(id==FavoritesTab),hover==id,pressed==id);}
        canvas.Fill({172,36,width-180,32},Surface,3); canvas.Stroke({172,36,width-180,32},Border,3);
        if(query.empty() && GetFocus()!=search) canvas.TextAt({176,38,width-220,28},L"Pesquisar músicas...",Muted);
        canvas.Button({width-36,38,28,28},L"",false,hover==ClearSearch,pressed==ClearSearch);
        EraserIcon(width-22,51,[&](float x1,float y1,float x2,float y2) {canvas.Line(x1,y1,x2,y2,Ui::Text,1.5f);});
        canvas.Fill({8,72,width-28,12},Surface);
        canvas.TextAt({12,70,24,14},L"#",Muted,Font::Small);
        canvas.TextAt({42,70,width-130,14},L"Título",Muted,Font::Small);
        canvas.TextAt({width-80,70,56,14},L"Duração",Muted,Font::Small,true);
        RECT list=List();
        Rect area{list.left/scale,list.top/scale,(list.right-list.left)/scale,(list.bottom-list.top)/scale};
        canvas.Fill(area,Surface); canvas.Clip(area);
        for(int row=top;row<view.Count() && row<top+Visible()+1;++row) {
            int index=view.Canonical(row); const Track& track=*PlayerPlaylist().At(index);
            float y=area.y+(row-top)*24;
            bool current=index==PlayerPlaylist().GetCurrentIndex();
            bool chosen=std::binary_search(selected.begin(),selected.end(),index);
            if(chosen || current) canvas.Fill({area.x,y,area.w,24},chosen?palette.selection:Active);
            canvas.Line(area.x,y+24,area.x+area.w,y+24,Background);
            canvas.TextAt({12,y,28,24},current?L"▶":std::to_wstring(index+1),current?Green:chosen?Ui::Text:Muted,Font::Small);
            canvas.TextAt({42,y,width-154,24},track.displayTitle,(chosen || current)?Ui::Text:track.failed?Danger:track.metadataPending?Muted:Ui::Text,Font::Body,false,current&&Audio_GetState()!=AUDIO_STATE_STOPPED);
            Star(canvas,width-98,y+12,track.favorite);
            wchar_t duration[32]=L"--:--";
            if(track.duration>=0) { unsigned seconds=static_cast<unsigned>(std::min(track.duration,359999.));
                swprintf(duration,ARRAYSIZE(duration),L"%u:%02u",seconds/60,seconds%60); }
            canvas.TextAt({width-80,y,56,24},duration,chosen?Ui::Text:Muted,Font::Small,true);
        }
        if(!view.Count()) canvas.TextAt(area,!query.empty()?L"Nenhuma música encontrada":favoritesOnly?L"Marque a estrela de uma música em Geral":L"Adicione arquivos ou uma pasta",Muted,Font::Body,true);
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
        canvas.TextAt({8,height-26,Button(AddMenu).left/scale-16,24},footer,Muted,Font::Small);
        if(canvas.End()) { EndPaint(hwnd,&ps); return; }
    }
    HDC dc = CreateCompatibleDC(target); HBITMAP bitmap = CreateCompatibleBitmap(target, r.right, r.bottom);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontW(-S(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, Ui::ActiveTheme().typography.body);
    HGDIOBJ oldFont = SelectObject(dc, font); SetBkMode(dc, TRANSPARENT);
    Fill(dc, r, Ui::NativeColor(Ui::Background)); Fill(dc, {0,0,r.right,S(24)}, Ui::NativeColor(Ui::Border));
    Text(dc, {S(6),0,r.right-S(25),S(24)}, L"PLAYLIST", Ui::NativeColor(Ui::Text));
    for (int id : {-1, static_cast<int>(AddMenu), static_cast<int>(Remove), static_cast<int>(ListMenu)}) {
        RECT b = Button(id);
        Fill(dc, b, pressed == id ? Ui::NativeColor(Ui::palette.pressed) : hover == id ? Ui::NativeColor(Ui::Hover) : Ui::NativeColor(Ui::Surface));
        if(id!=-1) {HBRUSH outline=CreateSolidBrush(Ui::NativeColor(Ui::Bronze));FrameRect(dc,&b,outline);DeleteObject(outline);}
        Text(dc, b, id == -1 ? L"×" : id == AddMenu ? L"ADIC ▾" : id == Remove ? L"REMOVER" : L"LISTA ▾",
            id == -1 ? Ui::NativeColor(Ui::Danger) : Ui::NativeColor(Ui::Text), DT_CENTER);
    }
    for(int id:{GeneralTab,FavoritesTab}){RECT b=Button(id);bool active=favoritesOnly==(id==FavoritesTab);
        Fill(dc,b,Ui::NativeColor(active?Ui::SelectedButtonFill():Ui::Surface));
        Text(dc,b,id==GeneralTab?L"Geral":L"Favoritas",Ui::NativeColor(active?Ui::SelectedButtonText:Ui::Text),DT_CENTER);}
    RECT clear=Button(ClearSearch);
    Fill(dc,clear,pressed==ClearSearch?Ui::NativeColor(Ui::Border):hover==ClearSearch?Ui::NativeColor(Ui::Hover):Ui::NativeColor(Ui::Surface));
    HPEN eraser=CreatePen(PS_SOLID,std::max(1,S(1)),Ui::NativeColor(Ui::Text));auto oldPen=SelectObject(dc,eraser);
    EraserIcon(r.right/scale-22,51,[&](float x1,float y1,float x2,float y2) {
        MoveToEx(dc,static_cast<int>(std::lround(x1*scale)),static_cast<int>(std::lround(y1*scale)),nullptr);
        LineTo(dc,static_cast<int>(std::lround(x2*scale)),static_cast<int>(std::lround(y2*scale)));
    });SelectObject(dc,oldPen);DeleteObject(eraser);
    RECT list = List(); Fill(dc, list, Ui::NativeColor(Ui::Surface));
    int saved = SaveDC(dc); IntersectClipRect(dc, list.left, list.top, list.right, list.bottom);
    HFONT boldFont=CreateFontW(-S(11),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,Ui::ActiveTheme().typography.body);
    for (int visibleRow = top; visibleRow < view.Count(); ++visibleRow) {
        int index=view.Canonical(visibleRow);
        RECT row = {list.left, list.top + (visibleRow-top)*S(24), list.right, list.top + (visibleRow-top+1)*S(24)};
        if (row.top >= list.bottom) break;
        const Track& track = *PlayerPlaylist().At(index);
        bool isSelected = std::binary_search(selected.begin(), selected.end(), index);
        if (isSelected) Fill(dc, row, Ui::NativeColor(Ui::palette.selection));
        std::wstring title = (index == PlayerPlaylist().GetCurrentIndex() ? L"> " : L"  ") + track.displayTitle;
        RECT name = row; name.left += S(3); name.right -= S(94);
        SelectObject(dc,index==PlayerPlaylist().GetCurrentIndex()&&Audio_GetState()!=AUDIO_STATE_STOPPED?boldFont:font);
        Text(dc, name, title, track.failed ? Ui::NativeColor(Ui::Danger) : track.metadataPending ? Ui::NativeColor(Ui::Muted) : Ui::NativeColor(Ui::Text));
        SelectObject(dc,font);
        RECT star{r.right-S(110),row.top,r.right-S(84),row.bottom};
        Text(dc,star,track.favorite?L"★":L"☆",Ui::NativeColor(track.favorite?Ui::Green:Ui::Muted),DT_CENTER);
        RECT duration = row; duration.left = duration.right - S(47); duration.right -= S(3);
        wchar_t time[32] = L"--:--";
        if (track.duration >= 0) {
            unsigned seconds = static_cast<unsigned>(std::min(track.duration, 359999.0));
            swprintf(time, ARRAYSIZE(time), L"%u:%02u", seconds/60, seconds%60);
        }
        Text(dc, duration, time, Ui::NativeColor(Ui::Muted), DT_RIGHT);
    }
    if (!view.Count()) Text(dc, list, favoritesOnly?L"Marque a estrela de uma música em Geral":L"Nenhuma música encontrada", Ui::NativeColor(Ui::Muted), DT_CENTER);
    if (dragging && insertion >= 0) {
        int y = list.top + (insertion-top)*S(24);
        Fill(dc, {list.left,y-S(1),list.right,y+S(1)}, Ui::NativeColor(Ui::Green));
    }
    RestoreDC(dc, saved);
    RECT rail = {list.right+S(3),list.top,r.right-S(4),list.bottom}; Fill(dc,rail,Ui::NativeColor(Ui::palette.sliderTrack));
    int count = view.Count();
    if (count > Visible()) {
        int height = rail.bottom - rail.top, thumb = std::max(S(12), height*Visible()/count);
        int y = rail.top + (height-thumb)*top/(count-Visible());
        Fill(dc, {rail.left,y,rail.right,y+thumb}, Ui::NativeColor(Ui::Bronze));
    }
    std::wstring footer = status;
    if (footer.empty()) footer = std::to_wstring(count) + L" faixas · " + std::to_wstring(selected.size()) +
        L" selecionadas" + (worker.Busy() ? L" · lendo..." : L"");
    Text(dc, {S(8),r.bottom-S(26),Button(AddMenu).left-S(8),r.bottom-S(2)}, footer, Ui::NativeColor(Ui::Muted));
    BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,oldFont); DeleteObject(font); DeleteObject(boldFont); SelectObject(dc,oldBitmap); DeleteObject(bitmap); DeleteDC(dc);
    EndPaint(hwnd,&ps);
}
LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    LRESULT menuResult{};
    if(Ui::ThemeMenuMessage(message,wp,lp,menuResult)) return menuResult;
    switch (message) {
    case WM_CREATE: window = hwnd; scale = MainWnd_DPIScaleFloat(1); DragAcceptFiles(hwnd,TRUE);
        resultsScheduled=false;
        query.clear(); favoritesOnly=Ui::LoadAppearance().playlistFavoritesOnly;starClick=false;contextId=0;view.Update(PlayerPlaylist(),query,favoritesOnly);
        searchBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        search=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,
            0,0,1,1,hwnd,reinterpret_cast<HMENU>(SearchId),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(search,EM_SETLIMITTEXT,512,0); LayoutSearch();
        SendMessageW(search,EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(L"Pesquisar músicas..."));
        if (!worker.Start(hwnd)) status = L"Falha ao iniciar leitura em segundo plano.";
        return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_SIZE: LayoutSearch(); ClampTop(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case Ui::WM_THEME_CHANGED:
        DeleteObject(searchBackground);searchBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        LayoutSearch();InvalidateRect(hwnd,nullptr,FALSE);InvalidateRect(search,nullptr,TRUE);return 0;
    case WM_CTLCOLOREDIT: SetTextColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Text));
        SetBkColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Surface)); return reinterpret_cast<LRESULT>(searchBackground);
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
        if(point.x>=S(172)&&point.y>=S(36) && point.y<S(68)) { ShowWindow(search,SW_SHOW); SetFocus(search); return 0; }
        RECT list=List();
        if (point.x>list.right && point.y>=list.top && point.y<list.bottom && view.Count()>Visible()) {
            scrollDrag=true; SetCapture(hwnd);
            top=(point.y-list.top)*(view.Count()-Visible())/std::max<int>(1,list.bottom-list.top);
            ClampTop(); PlWnd_Refresh(); return 0;
        }
        int row=HitRow(point); if (row<0) return 0;
        RECT client=Client();
        if(point.x>=client.right-S(110)&&point.x<client.right-S(84)){
            auto track=PlayerPlaylist().At(row);track->favorite=!track->favorite;starClick=true;SetCapture(hwnd);RefreshInformation();
            PlWnd_Refresh();return 0;}
        status.clear();
        bool ctrl=(GetKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetKeyState(VK_SHIFT)&0x8000)!=0;
        deferSelection = !ctrl && !shift && std::binary_search(selected.begin(),selected.end(),row);
        if (!deferSelection) {
            if(query.empty()&&!favoritesOnly) selected=SelectRows(selected,PlayerPlaylist().Count(),row,anchor,ctrl,shift);
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
        if (query.empty() && !favoritesOnly && !selected.empty() && (std::abs(point.x-down.x)>=S(4) || std::abs(point.y-down.y)>=S(4))) dragging=true;
        if (dragging) {
            if (point.y<list.top+S(10)) --top; else if (point.y>list.bottom-S(10)) ++top;
            ClampTop(); insertion=std::max(0,std::min<int>(PlayerPlaylist().Count(),top+(point.y-list.top+S(12))/S(24)));
            PlWnd_Refresh();
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if(starClick){starClick=false;if(GetCapture()==hwnd)ReleaseCapture();return 0;}
        bool moved=dragging, scrolled=scrollDrag; int button=pressed;
        dragging=false; scrollDrag=false; pressed=0;
        if (GetCapture()==hwnd) ReleaseCapture();
        if (button && HitButton(point)==button) {
            if (button==-1) { ShowWindow(hwnd,SW_HIDE); Ui::SaveAppearanceValue(L"PlaylistVisible",0); }
            else if (button==Remove) RemoveSelected();
            else if (button==AddFiles) PlWnd_AddFiles(hwnd);
            else if (button==AddFolder) FolderDialog();
            else if(button==GeneralTab||button==FavoritesTab){favoritesOnly=button==FavoritesTab;
                Ui::SaveAppearanceValue(L"PlaylistFavoritesOnly",favoritesOnly);top=0;selected.clear();anchor=-1;}
            else if (button==ClearSearch) { SetWindowTextW(search,L""); SetFocus(search); }
            else Menu(button==AddMenu,{Button(button).left,Button(button).top},true);
        } else if (moved && insertion>=0) {
            auto ids=SelectedIds(); PlayerPlaylist().MoveSelection(selected,insertion); RestoreSelection(ids);
        } else if (!scrolled && deferSelection) { int row=HitRow(point); if (row>=0) selected={row}; }
        insertion=-1; deferSelection=false; ClampTop(); PlWnd_Refresh(); return 0;
    }
    case WM_CAPTURECHANGED: starClick=false;dragging=false; scrollDrag=false; pressed=0; InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case WM_LBUTTONDBLCLK: {
        RECT client=Client();if(point.x>=client.right-S(110)&&point.x<client.right-S(84))return 0;
        int row=HitRow(point);
        if (row>=0 && !PlayerPlayback().PlayIndex(row,favoritesOnly)) Error(L"Não foi possível tocar a faixa selecionada.");
        PlWnd_Refresh(true); return 0;
    }
    case WM_MOUSEWHEEL: top-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*3; ClampTop(); PlWnd_Refresh(); return 0;
    case WM_KEYDOWN:
        if(wp==VK_TAB) { ShowWindow(search,SW_SHOW); SetFocus(search); }
        else if (wp==VK_DELETE) RemoveSelected();
        else if (wp=='A' && (GetKeyState(VK_CONTROL)&0x8000)) {
            status.clear();
            selected=view.Indices(); PlWnd_Refresh();
        } else if (wp==VK_RETURN && !selected.empty()) { PlayerPlayback().PlayIndex(selected.front(),favoritesOnly); PlWnd_Refresh(true); }
        else if((wp==VK_UP || wp==VK_DOWN || wp==VK_HOME || wp==VK_END) && view.Count()) {
            int row=view.Row(selected.empty()?-1:selected.back());
            row=wp==VK_HOME?0:wp==VK_END?view.Count()-1:std::clamp(row+(wp==VK_UP?-1:1),0,view.Count()-1);
            selected={view.Canonical(row)};anchor=selected.front();
            if(row<top) top=row;else if(row>=top+Visible()) top=row-Visible()+1;
            PlWnd_Refresh();
        }
        return 0;
    case WM_CONTEXTMENU: {
        int row=-1;
        if(point.x==-1&&point.y==-1){row=selected.empty()?-1:selected.front();RECT r=List();point={r.left,r.top+S(12)};}
        else {ScreenToClient(hwnd,&point);row=HitRow(point);}
        if(row>=0&&!std::binary_search(selected.begin(),selected.end(),row)){selected={row};anchor=row;PlWnd_Refresh();}
        Menu(false,point,false,row);return 0;}

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
        case TrackInfo: if(auto track=PlayerPlaylist().At(PlayerPlaylist().Find(contextId)))ShowInformation(*track);break;
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
        if(!resultsScheduled)resultsScheduled=SetTimer(hwnd,ResultsTimer,50,nullptr)!=0;
        if(resultsScheduled)return 0;
        [[fallthrough]];
    case WM_TIMER: {
        if(message==WM_TIMER&&wp!=ResultsTimer)break;
        KillTimer(hwnd,ResultsTimer);resultsScheduled=false;
        auto results=worker.Drain();
        if(results.empty()){InvalidateRect(hwnd,nullptr,FALSE);return 0;}
        for(auto& result:results) {
            if (result.generation!=worker.Generation()) continue;
            if (!result.error.empty()) { status=result.error; continue; }
            if (result.kind==PlaylistJob::Metadata) {
                if (auto track=PlayerPlaylist().At(PlayerPlaylist().Find(result.id))) {
                    track->metadata=result.metadata;
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
        RefreshInformation(); PlWnd_Refresh(); return 0;}
    case WM_CLOSE: ShowWindow(hwnd,SW_HIDE); Ui::SaveAppearanceValue(L"PlaylistVisible",0); return 0;
    case WM_DESTROY:
        KillTimer(hwnd,ResultsTimer);resultsScheduled=false;
        worker.Stop();
        if(information)DestroyWindow(information);
        if (!sessionPath.empty()) {
            std::wstring error;
            if (!SaveM3U(sessionPath,PlayerPlaylist().Tracks(),error))
                MessageBoxW(mainWindow,(L"Não foi possível preservar a playlist.\n"+error).c_str(),L"LiteSic",MB_OK|MB_ICONERROR);
            else {
                const int index=selected.empty()?-1:selected.front();
                const Track* track=PlayerPlaylist().At(index);
                std::wstring state=L"Index="+std::to_wstring(index)+L'\0'+L"Path="+(track?track->path:L"")+L'\0';
                state+=L'\0';
                // Profile APIs otherwise create ANSI files, losing non-ANSI paths.
                // This dedicated, optional sidecar contains only our selection section.
                const std::wstring ini=sessionPath+L".ini";
                HANDLE file=CreateFileW(ini.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
                const WORD bom=0xfeff;DWORD written=0;
                bool saved=file!=INVALID_HANDLE_VALUE && WriteFile(file,&bom,sizeof(bom),&written,nullptr) && written==sizeof(bom);
                if(file!=INVALID_HANDLE_VALUE) CloseHandle(file);
                if(!saved || !WritePrivateProfileSectionW(L"Selection",state.c_str(),ini.c_str()))
                    MessageBoxW(mainWindow,L"Não foi possível preservar a seleção da playlist.",L"LiteSic",MB_OK|MB_ICONWARNING);
                else if(!SaveGeometry(ini))
                    MessageBoxW(mainWindow,L"Não foi possível preservar a posição da playlist.",L"LiteSic",MB_OK|MB_ICONWARNING);
            }
        }
        sessionPath.clear(); search=nullptr; DeleteObject(searchFont); searchFont=nullptr;
        DeleteObject(searchBackground); searchBackground=nullptr; canvas.Reset(); window=nullptr; return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}

BOOL PlWnd_Create(HINSTANCE instance, HWND main, const wchar_t* sessionFile) {
    mainWindow=main;
    sessionPath=sessionFile?sessionFile:L"";
    selected.clear(); anchor=-1; top=0; status.clear();
    movingList=false; sizeMoveLoop=false; dock=Dock::None; dockAnchor=WindowSnap::Anchor::Start;
    WNDCLASSEXW cls{}; cls.cbSize=sizeof(cls); cls.style=CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW;
    cls.lpfnWndProc=Proc; cls.hInstance=instance; cls.hCursor=LoadCursorW(nullptr,IDC_ARROW); cls.lpszClassName=kClass;
    if (!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) return FALSE;
    scale=MainWnd_DPIScaleFloat(1);
    RECT parent{}; GetWindowRect(main,&parent); RECT work=WorkArea();
    int width=S(460),height=S(360),x=std::max(work.left,std::min(parent.left,work.right-width));
    int y=parent.bottom;
    Dock initial=Dock::Bottom;
    RECT restored{};bool hasGeometry=RestoreGeometry(restored,initial);
    if(hasGeometry){x=restored.left;y=restored.top;width=restored.right-restored.left;height=restored.bottom-restored.top;}
    else if (y+height>work.bottom) {
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
                    std::wstring ini=sessionPath+L".ini";
                    int index=static_cast<int>(GetPrivateProfileIntW(L"Selection",L"Index",-1,ini.c_str()));
                    wchar_t path[32768]{};
                    GetPrivateProfileStringW(L"Selection",L"Path",L"",path,ARRAYSIZE(path),ini.c_str());
                    if(auto track=PlayerPlaylist().At(index); track && track->path==path) {
                        selected={index};anchor=index;view.Update(PlayerPlaylist(),query,favoritesOnly);
                        top=view.Row(index)-Visible()/2;
                    }
                } else {
                    status=error;
                    // Do not replace an unreadable session with an empty list.
                    sessionPath.clear();
                }
            }
        }
        PlWnd_Refresh();
        RECT list{}; GetWindowRect(window,&list); RememberDock(initial,list);
        PlWnd_MainMoved(hasGeometry); ShowWindow(window,Ui::LoadAppearance().playlistVisible?SW_SHOWNOACTIVATE:SW_HIDE); UpdateWindow(window);
    }
    return window!=nullptr;
}
void PlWnd_Shutdown() { if (window) DestroyWindow(window); mainWindow=nullptr; dock=Dock::None; }
bool PlWnd_DockedRect(const RECT& owner,RECT& panel) {
    if(!window||movingList||dock==Dock::None||!IsWindowVisible(window))return false;
    GetWindowRect(window,&panel);WindowSnap::Place(panel,owner,{dock,dockAnchor});return true;
}
void PlWnd_MainMoved(bool preserveDock) {
    if (!window || !mainWindow || dock==Dock::None || movingList || IsIconic(mainWindow)) return;
    RECT parent{},list{}; GetWindowRect(mainWindow,&parent); GetWindowRect(window,&list);
    RECT aligned=list;
    WindowSnap::Dock attachment{dock,dockAnchor};
    RECT peer{};bool hasPeer=WindowSnap::OtherPanel(mainWindow,window,peer);
    if(preserveDock)WindowSnap::Place(aligned,parent,attachment);
    else WindowSnap::PlaceVisible(aligned,parent,attachment,WorkArea(),hasPeer?&peer:nullptr);
    dock=attachment.side;
    if (aligned.left!=list.left || aligned.top!=list.top)
        SetWindowPos(window,nullptr,aligned.left,aligned.top,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
void PlWnd_Show() { if(window) { ShowWindow(window,SW_SHOW); PlWnd_MainMoved(); Ui::SaveAppearanceValue(L"PlaylistVisible",1); SetForegroundWindow(window); } }
void PlWnd_Toggle() { if(window) { ShowWindow(window,IsWindowVisible(window)?SW_HIDE:SW_SHOWNOACTIVATE); PlWnd_MainMoved(); Ui::SaveAppearanceValue(L"PlaylistVisible",IsWindowVisible(window)!=FALSE); PlWnd_Refresh(); } }
bool PlWnd_IsVisible() { return window && IsWindowVisible(window); }
bool PlWnd_DialogMessage(MSG* message){
    if(!information||(message->hwnd!=information&&!IsChild(information,message->hwnd)))return false;
    if(message->message==WM_KEYDOWN&&message->wParam==VK_ESCAPE){DestroyWindow(information);return true;}
    return IsDialogMessageW(information,message)!=FALSE;
}
void PlWnd_Refresh(bool revealCurrent) {
    int current=PlayerPlaylist().GetCurrentIndex();
    view.Update(PlayerPlaylist(),query,favoritesOnly);
    if (revealCurrent && current>=0) {
        if (view.Row(current)<0 && search && !favoritesOnly) SetWindowTextW(search,L"");
        if(view.Row(current)>=0){selected={current}; anchor=current;}
        const int row=view.Row(current);
        const RECT list=List();
        // Even a partially visible last row is already in view. Only scroll
        // when the playing track is outside the current viewport.
        if(row>=0&&(row<top || (row-top)*S(24)>=list.bottom-list.top))
            top=row-Visible()/2;
    }
    ClampTop();
    selected.erase(std::remove_if(selected.begin(),selected.end(),[](int index) { return view.Row(index)<0; }),selected.end());
    if(view.Row(anchor)<0) anchor=selected.empty()?-1:selected.front();
    if(window) InvalidateRect(window,nullptr,FALSE);
    if(mainWindow) PostMessageW(mainWindow,WM_PLAYER_CHANGED,0,0);
}
bool PlWnd_PlaySelected() {
    int index=selected.empty()?-1:selected.front();
    bool ok=index>=0 && index!=PlayerPlaylist().GetCurrentIndex()
        ? PlayerPlayback().PlayIndex(index,favoritesOnly) : PlayerPlayback().Play(favoritesOnly);
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
