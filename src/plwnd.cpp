#include "ui_accessibility.h"
#include <oleacc.h>
#include "ui_scale.h"
#include "ui_tooltip.h"
#include "i18n.h"
#include "language_selection.h"
#include "plwnd.h"
#include "skin.h"
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
int keyboardFocus=0;
int anchor = -1, top = 0, insertion = -1, hover = 0, pressed = 0;
bool dragging = false, scrollDrag = false, deferSelection = false;
POINT down{};
float scale = 1,monitorDpi=1,informationScale=1,informationDpi=1;
using Dock = WindowSnap::Side;
auto& dock=Skin::Attachment(Skin::Playlist).side;
auto& dockAnchor=Skin::Attachment(Skin::Playlist).anchor;
// Keep semantic messages until painting, including worker errors queued before a language change.
struct StatusMessage {
    std::string id;
    void clear(){id.clear();}
    void Set(const std::string& value){id=value;}
    operator std::wstring() const{return id.empty()?L"":I18n::Text(id);}
} status;
std::wstring sessionPath;
int S(int n) { return static_cast<int>(std::lround(n * scale)); }
RECT Client() { RECT r{}; GetClientRect(window, &r); return r; }
RECT WorkArea() {
    MONITORINFO monitor{}; monitor.cbSize=sizeof(monitor);
    if (GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&monitor)) return monitor.rcWork;
    RECT work{}; SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0); return work;
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
    Ui::PanelPosition savedMain;if(Ui::LoadPlayerPosition(savedMain)){mx=owner.left;my=owner.top;}
    owner={mx,my,mx+owner.right-owner.left,my+owner.bottom-owner.top};
    MONITORINFO monitor{};monitor.cbSize=sizeof(monitor);
    if(!GetMonitorInfoW(MonitorFromRect(&owner,MONITOR_DEFAULTTONEAREST),&monitor))return false;
    WindowSnap::KeepVisible(owner,monitor.rcWork);
    panel={x,y,x+S(w),y+S(h)};side=static_cast<Dock>(d);dock=side;dockAnchor=static_cast<WindowSnap::Anchor>(a);
    Skin::RestoreParent(Skin::Playlist,read(L"Parent",Skin::Player));side=dock;
    if(side!=Dock::None&&Skin::Parent(Skin::Playlist)==Skin::Player){
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
    add(L"Dock",static_cast<int>(dock));add(L"Anchor",static_cast<int>(dockAnchor));add(L"Parent",Skin::Parent(Skin::Playlist));
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
        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,I18n::ScriptFont()?I18n::ScriptFont():Ui::ActiveTheme().typography.body);
    SendMessageW(search,WM_SETFONT,reinterpret_cast<WPARAM>(searchFont),TRUE);
}
void Fill(HDC dc, RECT r, COLORREF color) { HBRUSH brush = CreateSolidBrush(color); FillRect(dc, &r, brush); DeleteObject(brush); }
void Text(HDC dc, RECT r, const std::wstring& text, COLORREF color, UINT flags = DT_LEFT) {
    SetTextColor(dc, color);
    DrawTextW(dc, text.c_str(), static_cast<int>(text.size()), &r,
        flags | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX | (I18n::IsRtl()?DT_RTLREADING:0));
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
void Error(const char* id) { status.Set(id); PlWnd_Refresh(); }
void AddTrack(const Track& incoming) {
    if (PlayerPlaylist().Count() >= 100000) { status.Set("errors.track_limit"); return; }
    uint64_t id = PlayerPlaylist().Add(FullPath(incoming.path));
    Track* track = PlayerPlaylist().At(PlayerPlaylist().Count() - 1);
    if (!incoming.displayTitle.empty()) track->displayTitle = incoming.displayTitle;
    track->duration = incoming.duration;
    track->favorite = incoming.favorite;
    PlaylistJob job; job.id = id; job.path = track->path;
    if (!worker.Submit(std::move(job))) {
        track->metadataPending = false; status.Set("errors.metadata_unavailable");
    }
}
void AddPath(const std::wstring& path) {
    status.clear();
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        PlaylistJob job; job.kind = PlaylistJob::Scan; job.path = FullPath(path);
        if (!worker.Submit(std::move(job))) Error("errors.scan");
    } else if (IsSupportedAudioFile(path.c_str())) {
        Track track; track.path = path; AddTrack(track);
    }
}
void FolderDialog() {
    BROWSEINFOW info{}; info.hwndOwner = window;
    auto caption=I18n::Text("playlist.folder_dialog");info.lpszTitle=caption.c_str();
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
    std::wstring filter=I18n::Text("playlist.m3u_filter")+L'\0'+L"*.m3u;*.m3u8"+L'\0'+L'\0';
    info.lpstrFilter=filter.c_str();
    info.lpstrDefExt = L"m3u8"; auto caption=I18n::Text(save?"playlist.save_dialog":"playlist.open_dialog");info.lpstrTitle=caption.c_str();
    info.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    if (!(save ? GetSaveFileNameW(&info) : GetOpenFileNameW(&info))) return;
    PlaylistJob job; job.kind = save ? PlaylistJob::Save : PlaylistJob::Open; job.path = path;
    if (save) job.tracks = PlayerPlaylist().Tracks();
    // Opening appends; a failed open never destroys the current playlist.
    if (!worker.Submit(std::move(job))) Error("errors.m3u_start");
    else { status.Set(save?"playlist.saving":"playlist.opening"); PlWnd_Refresh(); }
}
void RemoveSelected() {
    for (auto it = selected.rbegin(); it != selected.rend(); ++it) PlayerPlayback().RemoveAt(*it);
    selected.clear(); anchor = -1; status.clear(); ClampTop(); PlWnd_Refresh();
}
void Menu(bool add, POINT point, bool upwards=false, int trackIndex=-1) {
    HMENU menu = CreatePopupMenu();
    if (add) {
        AppendMenuW(menu, MF_STRING, AddFiles, I18n::Text("playlist.files").c_str());
        AppendMenuW(menu, MF_STRING, AddFolder, I18n::Text("playlist.folders").c_str());
    } else {
        if(auto track=PlayerPlaylist().At(trackIndex)){contextId=track->id;AppendMenuW(menu,MF_STRING,TrackInfo,I18n::Text("playlist.info_menu").c_str());AppendMenuW(menu,MF_SEPARATOR,0,nullptr);}
        AppendMenuW(menu, MF_STRING, OpenList, I18n::Text("playlist.open_menu").c_str());
        AppendMenuW(menu, MF_STRING, SaveList, I18n::Text("playlist.save_menu").c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, NameAsc, I18n::Text("playlist.name_asc").c_str());
        AppendMenuW(menu, MF_STRING, NameDesc, I18n::Text("playlist.name_desc").c_str());
        AppendMenuW(menu, MF_STRING, DurationAsc, I18n::Text("playlist.duration_asc").c_str());
        AppendMenuW(menu, MF_STRING, DurationDesc, I18n::Text("playlist.duration_desc").c_str());
        AppendMenuW(menu, MF_STRING, Mix, I18n::Text("playlist.mix").c_str());
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, Clear, I18n::Text("playlist.clear").c_str());
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
    auto value=[](const std::wstring& s){return s.empty()?I18n::Text("info.unknown"):s;};
    std::wstring format=track.path.substr(track.path.find_last_of(L'.')+1);
    for(auto& c:format)c=std::towupper(c);
    std::wstring result;
    auto field=[&](const char* id,const std::wstring& data){
        if(!result.empty())result+=L"\r\n";
        result+=I18n::Format("info.field",{{"label",I18n::Text(id)},{"value",value(data)}});
    };
    field("info.title",m.title.empty()?track.displayTitle:m.title);
    field("info.artist",m.artist);field("info.album",m.album);field("info.genre",m.genre);
    field("info.year",m.year);field("info.track",m.trackNumber);field("info.format",format);
    wchar_t duration[48]{};
    if(track.duration>=0){unsigned seconds=static_cast<unsigned>(std::min(track.duration,359999.));swprintf(duration,48,L"%u:%02u",seconds/60,seconds%60);}
    field("info.duration",duration);
    field("info.sample_rate",m.sampleRate?std::to_wstring(m.sampleRate)+L" Hz":L"");
    field("info.channels",m.channels?std::to_wstring(m.channels):L"");
    field("info.size",std::to_wstring(m.fileBytes)+L" bytes");
    if(m.fileBytes&&track.duration>0)field("info.bitrate",std::to_wstring(static_cast<unsigned long long>(m.fileBytes*8./track.duration/1000))+L" kbps");
    field("info.favorite",I18n::Text(track.favorite?"common.yes":"common.no"));field("info.file",track.path);
    if(track.metadataPending)result+=L"\r\n\r\n"+I18n::Text("info.reading");
    else if(track.failed)result+=L"\r\n\r\n"+I18n::Text("errors.audio_read");
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
int IS(int value){return int(std::lround(value*informationScale));}
LRESULT CALLBACK InformationProc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    switch(msg){
    case WM_CREATE:
        information=hwnd;informationText.clear();
        informationEdit=CreateWindowExW(I18n::IsRtl()?WS_EX_RTLREADING:0,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|(I18n::IsRtl()?ES_RIGHT:0),
            IS(12),IS(12),IS(536),IS(328),hwnd,reinterpret_cast<HMENU>(801),GetModuleHandleW(nullptr),nullptr);
        SetWindowSubclass(informationEdit,InformationEditProc,1,0);
        Ui::Accessibility::NameNative(informationEdit,I18n::Text("info.caption"),I18n::Text("tooltip.info"));
        CreateWindowExW(0,L"BUTTON",I18n::Text("common.close").c_str(),WS_CHILD|WS_VISIBLE|WS_TABSTOP,IS(448),IS(348),IS(100),IS(28),hwnd,reinterpret_cast<HMENU>(802),GetModuleHandleW(nullptr),nullptr);
        Ui::Tooltips::Attach(hwnd,[](POINT){return Ui::Tooltips::Target{};},[](int){return std::wstring{};},[]{return informationScale;});
        Ui::Tooltips::AttachControl(informationEdit,"tooltip.info",[]{return informationScale;});
        Ui::Tooltips::AttachControl(GetDlgItem(hwnd,802),"tooltip.close",[]{return informationScale;});
        SendMessageW(hwnd,Ui::WM_THEME_CHANGED,0,0);RefreshInformation();return 0;
    case I18n::WM_LANGUAGE_CHANGED:{
        SetWindowTextW(hwnd,I18n::Text("info.caption").c_str());
        SetDlgItemTextW(hwnd,802,I18n::Text("common.close").c_str());
        auto top=SendMessageW(informationEdit,EM_GETFIRSTVISIBLELINE,0,0);
        DWORD start=0,end=0;SendMessageW(informationEdit,EM_GETSEL,reinterpret_cast<WPARAM>(&start),reinterpret_cast<LPARAM>(&end));
        I18n::UpdateEditDirection(informationEdit);Ui::Accessibility::NameNative(informationEdit,I18n::Text("info.caption"),I18n::Text("tooltip.info"));RefreshInformation();
        SendMessageW(informationEdit,EM_SETSEL,start,end);
        SendMessageW(informationEdit,EM_LINESCROLL,0,top-SendMessageW(informationEdit,EM_GETFIRSTVISIBLELINE,0,0));
        SendMessageW(hwnd,Ui::WM_THEME_CHANGED,0,0);return 0;}
    case Ui::WM_THEME_CHANGED:{
        HFONT old=informationFont;informationFont=CreateFontW(-IS(12),0,0,0,400,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,I18n::ScriptFont()?I18n::ScriptFont():Ui::ActiveTheme().typography.body);
        SendMessageW(informationEdit,WM_SETFONT,reinterpret_cast<WPARAM>(informationFont),TRUE);
        SendDlgItemMessageW(hwnd,802,WM_SETFONT,reinterpret_cast<WPARAM>(informationFont),TRUE);if(old)DeleteObject(old);
        if(informationBackground)DeleteObject(informationBackground);
        informationBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        InvalidateRect(hwnd,nullptr,TRUE);InvalidateRect(informationEdit,nullptr,TRUE);return 0;}
    case WM_DPICHANGED:{
        if(!lp)return 0;
        informationDpi=LOWORD(wp)/96.f;informationScale=Ui::FitScale(informationDpi*MainWnd_UserScaleMultiplier(),560,388,Ui::MonitorWork(*reinterpret_cast<RECT*>(lp)));
        RECT r=*reinterpret_cast<RECT*>(lp);DWORD style=DWORD(GetWindowLongPtrW(hwnd,GWL_STYLE));RECT size{0,0,IS(560),IS(388)};AdjustWindowRectEx(&size,style,FALSE,WS_EX_TOOLWINDOW);
        SetWindowPos(hwnd,nullptr,r.left,r.top,size.right-size.left,size.bottom-size.top,SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageW(hwnd,Ui::WM_THEME_CHANGED,0,0);return 0;
    }
    case WM_SIZE:{RECT r{};GetClientRect(hwnd,&r);MoveWindow(informationEdit,IS(12),IS(12),std::max<int>(1,r.right-IS(24)),std::max<int>(1,r.bottom-IS(60)),TRUE);
        MoveWindow(GetDlgItem(hwnd,802),r.right-IS(112),r.bottom-IS(40),IS(100),IS(28),TRUE);return 0;}
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
    informationDpi=Ui::WindowDpi(window);informationScale=scale;
    RECT parent{};GetWindowRect(window,&parent);RECT r{0,0,IS(560),IS(388)};
    DWORD style=WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN;AdjustWindowRectEx(&r,style,FALSE,WS_EX_TOOLWINDOW);
    HWND h=CreateWindowExW(WS_EX_TOOLWINDOW,cls.lpszClassName,I18n::Text("info.caption").c_str(),style,parent.left,parent.top,r.right-r.left,r.bottom-r.top,window,nullptr,cls.hInstance,nullptr);
    if(h){ShowWindow(h,SW_SHOWNOACTIVATE);SetFocus(informationEdit);}
}
std::wstring FooterText(){
    std::wstring footer=status;
        if(footer.empty()) {
            footer=I18n::Count("playlist.tracks",PlayerPlaylist().Count())+L" · "+I18n::Count("playlist.selected",selected.size());
            if(!query.empty()) footer+=L" · "+I18n::Count("playlist.visible",view.Count());
            if(worker.Busy()) footer+=(L" · "+I18n::Text("playlist.reading"));
            else if(PlayerPlaylist().Count()) {
                double seconds=0;bool known=true;
                for(const auto& track:PlayerPlaylist().Tracks()) {if(track.duration<0) known=false;else seconds+=track.duration;}
                if(known) { wchar_t duration[32]{};unsigned total=static_cast<unsigned>(std::min(seconds,359999.));
                    swprintf(duration,ARRAYSIZE(duration),L" · %u:%02u",total/60,total%60);footer+=duration; }
            }
        }
    return footer;
}
void Paint(HWND hwnd) {
    PAINTSTRUCT ps{}; HDC target = BeginPaint(hwnd, &ps); RECT r = Client();
    if(canvas.Begin(target,r,scale)) {
        using namespace Ui;
        const float width=r.right/scale,height=r.bottom/scale;
        canvas.Fill({0,0,width,height},Background);
        canvas.TitleBar(width,I18n::Text("playlist.title").c_str(),false,hover==-1?2:0,pressed==-1?2:0);
        for(int id : {static_cast<int>(AddMenu),static_cast<int>(Remove),static_cast<int>(ListMenu)}) {
            RECT b=Button(id);
            Rect button{b.left/scale,b.top/scale,(b.right-b.left)/scale,(b.bottom-b.top)/scale};
            canvas.Fill(button,pressed==id?Border:hover==id?Hover:Surface,3);
            canvas.Stroke(button,Bronze,3);
            canvas.TextAt(button,id==AddMenu?I18n::Text("playlist.add_short").c_str():id==Remove?I18n::Text("playlist.remove").c_str():I18n::Text("playlist.list_menu").c_str(),
                id==Remove && selected.empty()?Muted:Ui::Text,Font::Small,true);
        }
        for(int id:{GeneralTab,FavoritesTab}){RECT b=Button(id);canvas.Button({b.left/scale,b.top/scale,(b.right-b.left)/scale,(b.bottom-b.top)/scale},id==GeneralTab?I18n::Text("playlist.general").c_str():I18n::Text("playlist.favorites").c_str(),favoritesOnly==(id==FavoritesTab),hover==id,pressed==id);}
        canvas.Fill({172,36,width-180,32},Surface,3); canvas.Stroke({172,36,width-180,32},Border,3);
        if(query.empty() && GetFocus()!=search) canvas.TextAt({176,38,width-220,28},I18n::Text("playlist.search").c_str(),Muted);
        canvas.Button({width-36,38,28,28},L"",false,hover==ClearSearch,pressed==ClearSearch);
        EraserIcon(width-22,51,[&](float x1,float y1,float x2,float y2) {canvas.Line(x1,y1,x2,y2,Ui::Text,1.5f);});
        canvas.Fill({8,72,width-28,12},Surface);
        canvas.TextAt({12,70,24,14},L"#",Muted,Font::Small);
        canvas.TextAt({42,70,width-130,14},I18n::Text("info.title").c_str(),Muted,Font::Small);
        canvas.TextAt({width-80,70,56,14},I18n::Text("info.duration").c_str(),Muted,Font::Small,true);
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
        if(!view.Count()) canvas.TextAt(area,!query.empty()?I18n::Text("playlist.no_results").c_str():favoritesOnly?I18n::Text("playlist.favorite_hint").c_str():I18n::Text("playlist.empty").c_str(),Muted,Font::Body,true);
        if(dragging && insertion>=0) canvas.Line(area.x,area.y+(insertion-top)*24,area.x+area.w,area.y+(insertion-top)*24,Green,2);
        canvas.Unclip();
        if(GetFocus()==hwnd&&keyboardFocus){RECT focusRect=keyboardFocus==531?List():keyboardFocus>=1000?RECT{r.right-S(110),List().top+(view.Row(keyboardFocus-1000)-top)*S(24),r.right-S(84),List().top+(view.Row(keyboardFocus-1000)-top+1)*S(24)}:Button(keyboardFocus==1?-1:keyboardFocus);
            canvas.Stroke({focusRect.left/scale,focusRect.top/scale,(focusRect.right-focusRect.left)/scale,(focusRect.bottom-focusRect.top)/scale},Green,0,2);
        }
        canvas.Fill({width-16,area.y,8,area.h},Surface,3);
        if(view.Count()>Visible()) {
            float thumb=std::max(16.f,area.h*Visible()/view.Count());
            canvas.Fill({width-16,area.y+(area.h-thumb)*top/(view.Count()-Visible()),8,thumb},Bronze,3);
        }
        std::wstring footer=FooterText();
        canvas.TextAt({8,height-26,Button(AddMenu).left/scale-16,24},footer,Muted,Font::Small);
        if(canvas.End()) { EndPaint(hwnd,&ps); return; }
    }
    HDC dc = CreateCompatibleDC(target); HBITMAP bitmap = CreateCompatibleBitmap(target, r.right, r.bottom);
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HFONT font = CreateFontW(-S(11), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, I18n::ScriptFont()?I18n::ScriptFont():Ui::ActiveTheme().typography.body);
    HGDIOBJ oldFont = SelectObject(dc, font); SetBkMode(dc, TRANSPARENT);
    Fill(dc, r, Ui::NativeColor(Ui::Background)); Fill(dc, {0,0,r.right,S(24)}, Ui::NativeColor(Ui::Border));
    Text(dc, {S(6),0,r.right-S(25),S(24)}, I18n::Text("playlist.title").c_str(), Ui::NativeColor(Ui::Text));
    for (int id : {-1, static_cast<int>(AddMenu), static_cast<int>(Remove), static_cast<int>(ListMenu)}) {
        RECT b = Button(id);
        Fill(dc, b, pressed == id ? Ui::NativeColor(Ui::palette.pressed) : hover == id ? Ui::NativeColor(Ui::Hover) : Ui::NativeColor(Ui::Surface));
        if(id!=-1) {HBRUSH outline=CreateSolidBrush(Ui::NativeColor(Ui::Bronze));FrameRect(dc,&b,outline);DeleteObject(outline);}
        Text(dc, b, id == -1 ? L"×" : id == AddMenu ? I18n::Text("playlist.add_short").c_str() : id == Remove ? I18n::Text("playlist.remove").c_str() : I18n::Text("playlist.list_menu").c_str(),
            id == -1 ? Ui::NativeColor(Ui::Danger) : Ui::NativeColor(Ui::Text), DT_CENTER);
    }
    for(int id:{GeneralTab,FavoritesTab}){RECT b=Button(id);bool active=favoritesOnly==(id==FavoritesTab);
        Fill(dc,b,Ui::NativeColor(active?Ui::SelectedButtonFill():Ui::Surface));
        Text(dc,b,id==GeneralTab?I18n::Text("playlist.general").c_str():I18n::Text("playlist.favorites").c_str(),Ui::NativeColor(active?Ui::SelectedButtonText:Ui::Text),DT_CENTER);}
    RECT clear=Button(ClearSearch);
    Fill(dc,clear,pressed==ClearSearch?Ui::NativeColor(Ui::Border):hover==ClearSearch?Ui::NativeColor(Ui::Hover):Ui::NativeColor(Ui::Surface));
    HPEN eraser=CreatePen(PS_SOLID,std::max(1,S(1)),Ui::NativeColor(Ui::Text));auto oldPen=SelectObject(dc,eraser);
    EraserIcon(r.right/scale-22,51,[&](float x1,float y1,float x2,float y2) {
        MoveToEx(dc,static_cast<int>(std::lround(x1*scale)),static_cast<int>(std::lround(y1*scale)),nullptr);
        LineTo(dc,static_cast<int>(std::lround(x2*scale)),static_cast<int>(std::lround(y2*scale)));
    });SelectObject(dc,oldPen);DeleteObject(eraser);
    RECT list = List(); Fill(dc, list, Ui::NativeColor(Ui::Surface));
    int saved = SaveDC(dc); IntersectClipRect(dc, list.left, list.top, list.right, list.bottom);
    HFONT boldFont=CreateFontW(-S(11),0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,I18n::ScriptFont()?I18n::ScriptFont():Ui::ActiveTheme().typography.body);
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
    if (!view.Count()) Text(dc, list, favoritesOnly?I18n::Text("playlist.favorite_hint").c_str():I18n::Text("playlist.no_results").c_str(), Ui::NativeColor(Ui::Muted), DT_CENTER);
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
    if (footer.empty()) footer = I18n::Count("playlist.tracks",count) + L" · " + I18n::Count("playlist.selected",selected.size()) + (worker.Busy() ? (L" · "+I18n::Text("playlist.reading")) : L"");
    Text(dc, {S(8),r.bottom-S(26),Button(AddMenu).left-S(8),r.bottom-S(2)}, footer, Ui::NativeColor(Ui::Muted));
    BitBlt(target,0,0,r.right,r.bottom,dc,0,0,SRCCOPY);
    SelectObject(dc,oldFont); DeleteObject(font); DeleteObject(boldFont); SelectObject(dc,oldBitmap); DeleteObject(bitmap); DeleteDC(dc);
    EndPaint(hwnd,&ps);
}
Ui::Tooltips::Target TooltipHit(POINT p){
    int id=HitButton(p);RECT r{};
    if(id){r=Button(id);return {id==-1?1:id,r};}
    RECT client=Client();
    r={S(176),S(40),client.right-S(40),S(64)};if(PtInRect(&r,p))return {520,r};
    r={S(8),client.bottom-S(26),Button(AddMenu).left-S(8),client.bottom};if(PtInRect(&r,p))return {532,r};
    r=List();
    if(p.x>=client.right-S(18)&&p.x<client.right-S(8)&&p.y>=r.top&&p.y<r.bottom)return {530,{client.right-S(18),r.top,client.right-S(8),r.bottom}};
    int index=HitRow(p);if(index<0)return {};
    int y=r.top+(view.Row(index)-top)*S(24);
    if(p.x>=client.right-S(110)&&p.x<client.right-S(84))return {1000+index,{client.right-S(110),y,client.right-S(84),y+S(24)}};
    return {531,{r.left,y,r.right,y+S(24)}};
}
std::wstring TooltipText(int id){
    const char* key=nullptr;
    if(id==532)return FooterText();
    if(id>=1000){auto t=PlayerPlaylist().At(id-1000);return t?I18n::Text(t->favorite?"tooltip.favorite_off":"tooltip.favorite_on"):std::wstring{};}
    switch(id){
    case 1:key="tooltip.hide_playlist";break;case AddMenu:key="tooltip.add";break;
    case Remove:key="tooltip.remove";break;case ListMenu:key="tooltip.list_menu";break;
    case GeneralTab:key="tooltip.all";break;case FavoritesTab:key="tooltip.favorites";break;
    case ClearSearch:key="tooltip.clear_search";break;case SearchId:key="tooltip.search";break;
    case 530:key="tooltip.scroll";break;case 531:key="tooltip.row";break;
    }
    return key?I18n::Text(key):std::wstring{};
}
void FocusControl(int id){keyboardFocus=id;if(id==SearchId){ShowWindow(search,SW_SHOW);SetFocus(search);}else SetFocus(window);Ui::Accessibility::Focus(window,id);InvalidateRect(window,nullptr,FALSE);}
std::vector<int> FocusOrder(){std::vector<int> ids={GeneralTab,FavoritesTab,SearchId,ClearSearch,531};if(!selected.empty())ids.push_back(1000+selected.front());ids.insert(ids.end(),{AddMenu,Remove,ListMenu,1});return ids;}
void NextFocus(bool backwards){auto ids=FocusOrder();auto it=std::find(ids.begin(),ids.end(),keyboardFocus);int index=it==ids.end()?-1:int(it-ids.begin());FocusControl(ids[(index+(backwards?int(ids.size())-1:1)+int(ids.size()))%int(ids.size())]);}
void ActivateControl(int id){
    if(id>=1000){if(auto t=PlayerPlaylist().At(id-1000)){t->favorite=!t->favorite;RefreshInformation();PlWnd_Refresh();}return;}
    if(id==SearchId){FocusControl(id);return;}
    if(id==1){SendMessageW(window,WM_CLOSE,0,0);return;}
    if(id==531){if(!selected.empty())PlayerPlayback().PlayIndex(selected.front(),favoritesOnly);PlWnd_Refresh(true);return;}
    RECT r=Button(id==1?-1:id);LPARAM point=MAKELPARAM((r.left+r.right)/2,(r.top+r.bottom)/2);
    SendMessageW(window,WM_LBUTTONDOWN,0,point);SendMessageW(window,WM_LBUTTONUP,0,point);
}
LRESULT CALLBACK SearchKeyboard(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR){
    if(msg==WM_GETDLGCODE&&wp==VK_TAB)return DLGC_WANTTAB;
    if(msg==WM_KEYDOWN&&wp==VK_TAB){NextFocus(GetKeyState(VK_SHIFT)<0);return 0;}
    if(msg==WM_NCDESTROY)RemoveWindowSubclass(hwnd,SearchKeyboard,99);
    return DefSubclassProc(hwnd,msg,wp,lp);
}
void AttachAccessibility(HWND hwnd){Ui::Accessibility::Attach(hwnd,[hwnd]{std::vector<Ui::Accessibility::Control> controls;
    for(int id:std::initializer_list<int>{GeneralTab,FavoritesTab,ClearSearch,SearchId,AddMenu,Remove,ListMenu,1}){
        LONG state=STATE_SYSTEM_FOCUSABLE;if(GetFocus()==hwnd&&keyboardFocus==id)state|=STATE_SYSTEM_FOCUSED;
        if((id==GeneralTab&&!favoritesOnly)||(id==FavoritesTab&&favoritesOnly))state|=STATE_SYSTEM_SELECTED;
        if(id==Remove&&selected.empty())state|=STATE_SYSTEM_UNAVAILABLE;
        auto text=TooltipText(id);RECT bounds=Button(id==1?-1:id);if(id==SearchId){GetWindowRect(search,&bounds);MapWindowPoints(nullptr,hwnd,reinterpret_cast<POINT*>(&bounds),2);}
        controls.push_back({id,bounds,text,text,L"",id==SearchId?ROLE_SYSTEM_TEXT:id==GeneralTab||id==FavoritesTab?ROLE_SYSTEM_PAGETAB:ROLE_SYSTEM_PUSHBUTTON,state});
    }
    RECT r=List(),client=Client();auto footer=FooterText();controls.push_back({532,{S(8),client.bottom-S(26),Button(AddMenu).left-S(8),client.bottom},footer,footer,L"",ROLE_SYSTEM_STATICTEXT,0});for(int row=top;row<std::min(view.Count(),top+Visible());++row){int index=view.Canonical(row);auto t=PlayerPlaylist().At(index);int y=r.top+(row-top)*S(24);
        LONG state=STATE_SYSTEM_FOCUSABLE;if(std::binary_search(selected.begin(),selected.end(),index))state|=STATE_SYSTEM_SELECTED;
        if(GetFocus()==hwnd&&keyboardFocus==531&&!selected.empty()&&selected.front()==index)state|=STATE_SYSTEM_FOCUSED;
        controls.push_back({200000+index,{r.left,y,client.right-S(110),y+S(24)},t->displayTitle,TooltipText(531),L"",ROLE_SYSTEM_LISTITEM,state});
        LONG star=STATE_SYSTEM_FOCUSABLE|(t->favorite?STATE_SYSTEM_CHECKED:0);if(GetFocus()==hwnd&&keyboardFocus==1000+index)star|=STATE_SYSTEM_FOCUSED;
        auto text=TooltipText(1000+index);controls.push_back({1000+index,{client.right-S(110),y,client.right-S(84),y+S(24)},text,text,L"",ROLE_SYSTEM_CHECKBUTTON,star});
    }return controls;
},[](int id){if(id>=200000){PlayerPlayback().PlayIndex(id-200000,favoritesOnly);PlWnd_Refresh(true);}else ActivateControl(id);},[](int id){if(id>=200000){selected={id-200000};anchor=selected.front();FocusControl(531);}else FocusControl(id);});}
LRESULT CALLBACK Proc(HWND hwnd, UINT message, WPARAM wp, LPARAM lp) {
    POINT point{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
    LRESULT menuResult{};
    if(Ui::ThemeMenuMessage(message,wp,lp,menuResult)) return menuResult;
    switch (message) {
    case WM_CREATE: window = hwnd; monitorDpi=Ui::WindowDpi(hwnd);scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,360,Ui::MonitorWork(hwnd));
        {auto create=reinterpret_cast<CREATESTRUCTW*>(lp);float initial=MainWnd_DPIScaleFloat(1);SetWindowPos(hwnd,nullptr,0,0,int(std::lround(create->cx/initial*scale)),int(std::lround(create->cy/initial*scale)),SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);}DragAcceptFiles(hwnd,TRUE);
        resultsScheduled=false;keyboardFocus=0;
        query.clear(); favoritesOnly=Ui::LoadAppearance().playlistFavoritesOnly;starClick=false;contextId=0;view.Update(PlayerPlaylist(),query,favoritesOnly);
        searchBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        search=CreateWindowExW(0,L"EDIT",L"",WS_CHILD|WS_TABSTOP|ES_AUTOHSCROLL,
            0,0,1,1,hwnd,reinterpret_cast<HMENU>(SearchId),GetModuleHandleW(nullptr),nullptr);
        SetWindowSubclass(search,SearchKeyboard,99,0);
        AttachAccessibility(hwnd);
        I18n::UpdateEditDirection(search);Ui::Accessibility::NameNative(search,I18n::Text("playlist.search"),I18n::Text("tooltip.search"));
        SendMessageW(search,EM_SETLIMITTEXT,512,0); LayoutSearch();
        SendMessageW(search,EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(I18n::Text("playlist.search").c_str()));
        Ui::Tooltips::Attach(hwnd,TooltipHit,TooltipText,[]{return scale;});
        Ui::Tooltips::AttachControl(search,"tooltip.search",[]{return scale;});
        if (!worker.Start(hwnd)) status.Set("errors.worker");
        return 0;
    case WM_SETFOCUS:case WM_KILLFOCUS:InvalidateRect(hwnd,nullptr,FALSE);return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: Paint(hwnd); return 0;
    case WM_SIZE: LayoutSearch(); ClampTop(); InvalidateRect(hwnd,nullptr,FALSE); return 0;
    case I18n::WM_LANGUAGE_CHANGED:
        SetWindowTextW(hwnd,I18n::Format("window.caption",{{"title",I18n::Text("playlist.title")}}).c_str());
        SendMessageW(search,EM_SETCUEBANNER,FALSE,reinterpret_cast<LPARAM>(I18n::Text("playlist.search").c_str()));
        I18n::UpdateEditDirection(search);Ui::Accessibility::NameNative(search,I18n::Text("playlist.search"),I18n::Text("tooltip.search"));LayoutSearch();
        InvalidateRect(hwnd,nullptr,FALSE);InvalidateRect(search,nullptr,TRUE);return 0;
    case Ui::WM_THEME_CHANGED:
        DeleteObject(searchBackground);searchBackground=CreateSolidBrush(Ui::NativeColor(Ui::Surface));
        LayoutSearch();InvalidateRect(hwnd,nullptr,FALSE);InvalidateRect(search,nullptr,TRUE);return 0;
    case WM_CTLCOLOREDIT: SetTextColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Text));
        SetBkColor(reinterpret_cast<HDC>(wp),Ui::NativeColor(Ui::Surface)); return reinterpret_cast<LRESULT>(searchBackground);
    case WM_ENTERSIZEMOVE:Skin::BeginMove(Skin::Playlist);return 0;
    case WM_MOVING:Skin::Moving(Skin::Playlist,*reinterpret_cast<RECT*>(lp),S(24));return TRUE;
    case WM_EXITSIZEMOVE:Skin::EndMove(Skin::Playlist,S(24));return 0;
    case WM_WINDOWPOSCHANGING:Skin::Constrain(Skin::Playlist,*reinterpret_cast<WINDOWPOS*>(lp));break;
    case WM_WINDOWPOSCHANGED:Skin::Changed(Skin::Playlist);break;
    case WM_DISPLAYCHANGE:Skin::Recover();return 0;
    case WM_GETMINMAXINFO: {
        auto info = reinterpret_cast<MINMAXINFO*>(lp);
        info->ptMinTrackSize = {S(275),S(180)}; return 0;
    }
    case WM_DPICHANGED: {
        if(!lp)return 0;
        RECT oldClient=Client();float oldScale=scale;
        monitorDpi=LOWORD(wp)/96.f;scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,360,Ui::MonitorWork(*reinterpret_cast<RECT*>(lp)));
        auto suggested = reinterpret_cast<RECT*>(lp);
        SetWindowPos(hwnd,nullptr,suggested->left,suggested->top,int(std::lround(oldClient.right/oldScale*scale)),
            int(std::lround(oldClient.bottom/oldScale*scale)),SWP_NOZORDER|SWP_NOACTIVATE);
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
        SetFocus(hwnd); pressed = HitButton(point);keyboardFocus=pressed==-1?1:pressed?pressed:531;
        if (pressed) { SetCapture(hwnd); InvalidateRect(hwnd,nullptr,FALSE); return 0; }
        if(point.x>=S(172)&&point.y>=S(36) && point.y<S(68)) { FocusControl(SearchId); return 0; }
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
        keyboardFocus=531;
        RECT client=Client();if(point.x>=client.right-S(110)&&point.x<client.right-S(84))return 0;
        int row=HitRow(point);
        if (row>=0 && !PlayerPlayback().PlayIndex(row,favoritesOnly)) Error("errors.play_selected");
        PlWnd_Refresh(true); return 0;
    }
    case WM_MOUSEWHEEL: top-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*3; ClampTop(); PlWnd_Refresh(); return 0;
    case WM_APPCOMMAND:return SendMessageW(mainWindow,WM_APPCOMMAND,wp,lp);
    case WM_KEYDOWN:
        {MSG key{hwnd,WM_KEYDOWN,wp,lp,0,{0,0}};if(MainWnd_TranslateShortcut(&key))return 0;}
        if(wp==VK_TAB) NextFocus(GetKeyState(VK_SHIFT)<0);
        else if((wp==VK_RETURN||wp==VK_SPACE)&&keyboardFocus&&keyboardFocus!=531)ActivateControl(keyboardFocus);
        else if (wp==VK_DELETE) RemoveSelected();
        else if (wp=='A' && (GetKeyState(VK_CONTROL)&0x8000)) {
            status.clear();
            selected=view.Indices(); PlWnd_Refresh();
        } else if (wp==VK_RETURN && !selected.empty()) { PlayerPlayback().PlayIndex(selected.front(),favoritesOnly); PlWnd_Refresh(true); }
        else if((wp==VK_UP || wp==VK_DOWN || wp==VK_HOME || wp==VK_END) && view.Count()) {
            keyboardFocus=531;
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
            if (!result.error.empty()) { status.Set(result.errorId); continue; }
            if (result.kind==PlaylistJob::Metadata) {
                if (auto track=PlayerPlaylist().At(PlayerPlaylist().Find(result.id))) {
                    track->metadata=result.metadata;
                    track->metadataPending=false; track->metadataLoaded=true; track->failed=!result.metadata.readable;
                    if (result.metadata.duration>=0) track->duration=result.metadata.duration;
                    if (!result.metadata.title.empty()) track->displayTitle=result.metadata.artist.empty() ? result.metadata.title :
                        result.metadata.artist+L" — "+result.metadata.title;
                }
            } else if (result.kind==PlaylistJob::Save) status.Set("playlist.saved");
            else {
                if (result.kind==PlaylistJob::Open) status.Set("playlist.added");
                for(const auto& track:result.tracks) AddTrack(track);
            }
        }
        RefreshInformation(); PlWnd_Refresh(); return 0;}
    case WM_CLOSE: ShowWindow(hwnd,SW_HIDE);Skin::Reconcile(); Ui::SaveAppearanceValue(L"PlaylistVisible",0); return 0;
    case WM_DESTROY:
        KillTimer(hwnd,ResultsTimer);resultsScheduled=false;
        worker.Stop();
        if(information)DestroyWindow(information);
        if (!sessionPath.empty()) {
            std::wstring error;
            if (!SaveM3U(sessionPath,PlayerPlaylist().Tracks(),error))
                MessageBoxW(mainWindow,I18n::Format("errors.preserve_playlist",{{"error",error}}).c_str(),L"LiteSic",MB_OK|MB_ICONERROR);
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
                    MessageBoxW(mainWindow,I18n::Text("errors.preserve_selection").c_str(),L"LiteSic",MB_OK|MB_ICONWARNING);
                else if(!SaveGeometry(ini))
                    MessageBoxW(mainWindow,I18n::Text("errors.preserve_position").c_str(),L"LiteSic",MB_OK|MB_ICONWARNING);
            }
        }
        sessionPath.clear(); search=nullptr; DeleteObject(searchFont); searchFont=nullptr;
        DeleteObject(searchBackground); searchBackground=nullptr; canvas.Reset();Skin::Unbind(Skin::Playlist); window=nullptr; return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
}

BOOL PlWnd_Create(HINSTANCE instance, HWND main, const wchar_t* sessionFile) {
    mainWindow=main;
    sessionPath=sessionFile?sessionFile:L"";
    selected.clear(); anchor=-1; top=0; status.clear();
    Skin::Bind(Skin::Player,main);dock=Dock::None;Skin::RestoreParent(Skin::Playlist,Skin::Player); dockAnchor=WindowSnap::Anchor::Start;
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
    window=CreateWindowExW(WS_EX_ACCEPTFILES,kClass,I18n::Format("window.caption",{{"title",I18n::Text("playlist.title")}}).c_str(),WS_POPUP,
        x,y,width,height,main,nullptr,instance,nullptr);
    if (window) {
        Skin::Bind(Skin::Playlist,window);
        if (!sessionPath.empty()) {
            DWORD attributes=GetFileAttributesW(sessionPath.c_str());
            if (attributes!=INVALID_FILE_ATTRIBUTES || GetLastError()!=ERROR_FILE_NOT_FOUND) {
                std::vector<Track> tracks; std::wstring error;std::string errorId;
                if (LoadM3U(sessionPath,tracks,error,&errorId)) {
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
                    status.Set(errorId);
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
void PlWnd_Shutdown() { if (window) DestroyWindow(window);Skin::Unbind(Skin::Playlist); mainWindow=nullptr; dock=Dock::None; }
bool PlWnd_DockedRect(const RECT& owner,RECT& panel){return Skin::DockedRect(Skin::Playlist,owner,panel);}
void PlWnd_MainMoved(bool preserveDock){if(preserveDock)Skin::Reconcile();else Skin::Recover();}
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
    if(information){RECT r{};GetWindowRect(information,&r);SendMessageW(information,WM_DPICHANGED,MAKELONG(int(informationDpi*96),int(informationDpi*96)),reinterpret_cast<LPARAM>(&r));}
    float old=scale; scale=Ui::FitScale(monitorDpi*MainWnd_UserScaleMultiplier(),460,360,Ui::MonitorWork(window)); RECT r=Client();
    SetWindowPos(window,nullptr,0,0,static_cast<int>(r.right/old*scale),static_cast<int>(r.bottom/old*scale),
        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    if (dock!=Dock::None) PlWnd_MainMoved(); else AvoidOverlap();
    ClampTop(); PlWnd_Refresh();
}
void PlWnd_AddFiles(HWND owner, bool playFirst) {
    std::vector<wchar_t> buffer(32768);
    OPENFILENAMEW info{}; info.lStructSize=sizeof(info); info.hwndOwner=owner;
    info.lpstrFile=buffer.data(); info.nMaxFile=static_cast<DWORD>(buffer.size());
    std::wstring filter=I18n::Text("playlist.audio_filter")+L'\0'+L"*.mp3;*.wav;*.flac;*.ogg"+L'\0'+L'\0';
    info.lpstrFilter=filter.c_str();
    info.Flags=OFN_EXPLORER|OFN_ALLOWMULTISELECT|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&info)) {
        if (CommDlgExtendedError()==FNERR_BUFFERTOOSMALL) Error("errors.selection_large");
        return;
    }
    int first=PlayerPlaylist().Count();
    std::wstring base=buffer.data(); const wchar_t* next=buffer.data()+base.size()+1;
    if (!*next) AddPath(base);
    else while(*next) { AddPath(base+L"\\"+next); next+=wcslen(next)+1; }
    status.clear();
    if (playFirst && first<PlayerPlaylist().Count() && !PlayerPlayback().PlayIndex(first)) Error("errors.play_file");
    PlWnd_Refresh();
}

bool PlWnd_AppendPaths(const std::vector<std::wstring>& paths){
    if(paths.size()+PlayerPlaylist().Count()>100000)return false;
    for(const auto& path:paths){DWORD attrs=GetFileAttributesW(path.c_str());if(attrs==INVALID_FILE_ATTRIBUTES||(!(attrs&FILE_ATTRIBUTE_DIRECTORY)&&!IsSupportedAudioFile(path.c_str())))return false;}
    for(const auto& path:paths)AddPath(path);
    if(!paths.empty())PlWnd_Refresh();
    return true;
}
