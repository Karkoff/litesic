#include "ui_theme.h"
#include <shlobj.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <initializer_list>

namespace Ui {
namespace {
double Luminance(uint32_t c) {
    auto linear=[](unsigned v) { double s=v/255.;return s<=.04045?s/12.92:std::pow((s+.055)/1.055,2.4); };
    return .2126*linear((c>>16)&255)+.7152*linear((c>>8)&255)+.0722*linear(c&255);
}
uint32_t Mix(uint32_t a,uint32_t b,double t) {
    uint32_t result=0;
    for(int shift : {0,8,16}) result|=static_cast<uint32_t>(std::lround(((a>>shift)&255)*(1-t)+((b>>shift)&255)*t))<<shift;
    return result;
}
uint32_t Readable(uint32_t color,const std::vector<uint32_t>& backgrounds,double ratio) {
    auto valid=[&](uint32_t c) { for(auto b:backgrounds) if(Contrast(c,b)<ratio) return false;return true; };
    if(valid(color)) return color;
    uint32_t end=Luminance(backgrounds[0])>.35?0x000000:0xFFFFFF;
    for(int i=1;i<=100;++i) { auto c=Mix(color,end,i/100.);if(valid(c)) return c; }
    return end;
}
Theme Make(const wchar_t* id,const wchar_t* name,const wchar_t* category,
           uint32_t bg,uint32_t surface,uint32_t display,uint32_t tint,uint32_t accent,
           uint32_t secondary,bool light=false,const wchar_t* font=L"Segoe UI",bool italic=false) {
    uint32_t text=light?0x192128:0xF0EEE8, muted=light?0x425058:0xB6B9B8;
    uint32_t hover=Mix(surface,tint,.18),active=Mix(surface,accent,.17),border=Mix(surface,tint,.38);
    std::vector<uint32_t> backgrounds{bg,surface,hover,active,border,display};
    text=Readable(text,backgrounds,4.5);muted=Readable(muted,backgrounds,4.5);
    accent=Readable(accent,backgrounds,4.5);secondary=Readable(secondary,backgrounds,3);
    auto danger=Readable(light?0xA71E34:0xFF7F87,backgrounds,4.5);
    ThemePalette p{bg,surface,border,secondary,accent,secondary,text,muted,danger,display,active,hover,
        border,active,border,accent,accent,secondary,Readable(0xF8BF50,backgrounds,4.5)};
    return {id,name,category,p,{font,L"Consolas",{12,10,14,27},{400,400,600,600},italic}};
}
const std::vector<Theme>& Catalog() {
    static const std::vector<Theme> themes={
        {L"original",L"LiteSic Original",L"Essenciais",{0x3B2820,0x2B2018,0x5A4633,0x887B5B,0x00E676,0xC4FF00,0xE6E1D7,0xA8A096,0xFF6668,0x10140F,0x23402B,0x513C2C,0x5A4633,0x5A4633,0x5A4633,0x00E676,0x00E676,0x00E676,0xFFCC66},{L"Segoe UI",L"Consolas",{12,10,14,27},{400,400,600,600},false}},
        Make(L"dark",L"Escuro",L"Essenciais",0x14171C,0x20252C,0x0B0E12,0x8E9AA8,0x67D5A3,0x9EC4FF),
        Make(L"light",L"Claro",L"Essenciais",0xE9EDF2,0xFAFCFF,0xE1E8EF,0x576B80,0x006B50,0x235CB0,true),
        Make(L"inverse",L"Inverso",L"Essenciais",0xE4D6C5,0xF6EBDD,0xD5C4AC,0x6E472C,0x33581A,0x643794,true),
        Make(L"high-contrast",L"Alto Contraste",L"Essenciais",0,0x080808,0,0xFFFFFF,0xFFFF00,0x00FFFF),
        Make(L"mono",L"Monocromático",L"Essenciais",0x1A1A1A,0x292929,0x090909,0xCCCCCC,0xEEEEEE,0xBBBBBB),
        Make(L"summer",L"Verão",L"Sazonais",0xFFF0CD,0xFFF9E8,0xE6F2E8,0x99602B,0x007361,0xA34C10,true),
        Make(L"winter",L"Inverno",L"Sazonais",0x172D42,0x203D55,0x0C2033,0xB7DCEF,0x86E4FF,0xD1C6FA),
        Make(L"spring",L"Primavera",L"Sazonais",0xE4F2D8,0xF5FAEB,0xD8EACB,0x567836,0x32671E,0x9B3265,true),
        Make(L"autumn",L"Outono",L"Sazonais",0x39251E,0x482F24,0x251912,0xC48E4C,0xFFBA69,0xDBCB83),
        Make(L"tropical",L"Tropical",L"Sazonais",0x073F37,0x0D4C41,0x032B25,0xC4C341,0xBFEF64,0xFFD083),
        Make(L"aurora",L"Aurora Boreal",L"Sazonais",0x101D36,0x162B43,0x080F24,0x664F9A,0x6FF3C0,0xD6A7FF),
        Make(L"christmas",L"Natal",L"Comemorativos",0x301819,0x1C3026,0x101D17,0xBD8E4E,0xFFE4A0,0xFF9E9E),
        Make(L"new-year",L"Ano Novo",L"Comemorativos",0x171D2A,0x252C3A,0x0A101C,0xB99B60,0xFFE5A0,0xB9E6FF),
        Make(L"carnival",L"Carnaval",L"Comemorativos",0x32143E,0x421C50,0x1D0B2C,0xAA458E,0xFFE16C,0x6AF6DE),
        Make(L"easter",L"Páscoa",L"Comemorativos",0xEFE3F5,0xFAF1FF,0xE5D6EA,0x805790,0x713E88,0x24684E,true),
        Make(L"halloween",L"Halloween",L"Comemorativos",0x211626,0x302037,0x100C15,0x9B5930,0xFFAD50,0xCBA1FF),
        Make(L"june-festival",L"Festa Junina",L"Comemorativos",0x432B1C,0x503324,0x271B11,0xBA6A3E,0xFFE19A,0xB5E4A8),
        Make(L"valentines",L"Dia dos Namorados",L"Comemorativos",0x381D2D,0x492639,0x241020,0xB76580,0xFFBDD4,0xE8C4FF),
        Make(L"children",L"Dia das Crianças",L"Comemorativos",0xE4EFFC,0xF3F8FF,0xD9E8FA,0x537CB4,0x23549A,0x9F326A,true),
        Make(L"cyberpunk",L"Cyberpunk",L"Tecnológicos",0x1A1233,0x241A45,0x0D0920,0x754798,0xFFEA4D,0x57F4F4),
        Make(L"matrix",L"Matrix",L"Tecnológicos",0x08190E,0x102519,0x020B05,0x357348,0x73FF97,0xC0FFBD,false,L"Consolas"),
        Make(L"neon-blue",L"Neon Blue",L"Tecnológicos",0x091D35,0x102A47,0x040F22,0x346B9C,0x6ADEFF,0xAABEFF),
        Make(L"neon-purple",L"Neon Purple",L"Tecnológicos",0x201136,0x301C4C,0x130923,0x8155A6,0xE0A0FF,0xFFA9DF),
        Make(L"terminal",L"Terminal Retro",L"Tecnológicos",0x19150B,0x282112,0x0B0904,0x7C6332,0xFFD778,0xEAE0B2,false,L"Consolas"),
        Make(L"synthwave",L"Synthwave",L"Tecnológicos",0x28132F,0x3A1B40,0x140B25,0x8C467A,0xFF9AD9,0x79E5FF),
        Make(L"midnight",L"Midnight",L"Tecnológicos",0x0D1429,0x171F39,0x050B1B,0x4B608F,0xA6BCFF,0xB5E7F0),
        Make(L"carbon",L"Carbon",L"Tecnológicos",0x202427,0x2C3235,0x111719,0x667879,0xB2DFD4,0xD6C4A7),
        Make(L"ocean",L"Oceano",L"Naturais",0x083540,0x104650,0x04242E,0x418A91,0xA1F0E0,0x9CD8FF),
        Make(L"forest",L"Floresta",L"Naturais",0x192D20,0x263D2B,0x0E1E15,0x6B8D58,0xC2E99C,0xE8CE95),
        Make(L"desert",L"Deserto",L"Naturais",0xEDDCBF,0xF9EACD,0xE2CCA8,0x906639,0x77431C,0x325C60,true,L"Tahoma"),
        Make(L"mountain",L"Montanha",L"Naturais",0x29353D,0x35434B,0x18252C,0x849CA0,0xB6E6DB,0xDFD1B6),
        Make(L"sunset",L"Pôr do Sol",L"Naturais",0x3D2032,0x4C2B3D,0x261426,0xB26E65,0xFFD094,0xE9B0E9),
        Make(L"sakura",L"Sakura",L"Naturais",0xF3E2E8,0xFFF1F5,0xE8D4DC,0x9F637D,0x943556,0x506532,true,L"Segoe UI",true),
        Make(L"winamp",L"Winamp Classic",L"Clássicos",0x252633,0x323443,0x080F0B,0x9697A4,0xBDFF74,0xB8D8FF,false,L"Tahoma"),
        Make(L"vintage",L"Vintage Audio",L"Clássicos",0x382B20,0x47382A,0x201B12,0xAD976A,0xF3D89B,0xD5E4A7,false,L"Tahoma")
    };
    return themes;
}
size_t current=0;
unsigned generation=1;
std::wstring path;
BOOL CALLBACK Notify(HWND hwnd,LPARAM) {
    wchar_t cls[80]{};GetClassNameW(hwnd,cls,80);
    if(wcsncmp(cls,L"LiteSic.",8)==0) SendMessageW(hwnd,WM_THEME_CHANGED,0,0);
    return TRUE;
}
}
double Contrast(uint32_t a,uint32_t b) { double x=Luminance(a),y=Luminance(b);return (std::max(x,y)+.05)/(std::min(x,y)+.05); }
size_t ThemeCount() { return Catalog().size(); }
const Theme& ThemeAt(size_t index) { return Catalog().at(index); }
const Theme& ActiveTheme() { return ThemeAt(current); }
unsigned ThemeGeneration() { return generation; }
bool SelectTheme(const wchar_t* id,bool save) {
    size_t chosen=0;
    if(id) for(size_t i=0;i<ThemeCount();++i) if(!wcscmp(id,ThemeAt(i).id)) { chosen=i;break; }
    if(chosen!=current) {
        current=chosen;palette=ActiveTheme().palette;++generation;
        EnumThreadWindows(GetCurrentThreadId(),Notify,0);
    }
    return !save || path.empty() || WritePrivateProfileStringW(L"Appearance",L"Theme",ActiveTheme().id,path.c_str());
}
bool RestoreTheme() { return SelectTheme(L"original"); }
AppearanceState LoadAppearance() {
    AppearanceState state;
    auto choice=[](const wchar_t* key,int fallback,std::initializer_list<int> allowed) {
        if(path.empty()) return fallback;
        wchar_t value[32]{};
        GetPrivateProfileStringW(L"Appearance",key,L"",value,32,path.c_str());
        for(int candidate:allowed) if(std::to_wstring(candidate)==value) return candidate;
        return fallback;
    };
    state.visualizerVisible=choice(L"VisualizerVisible",1,{0,1})!=0;
    state.playlistVisible=choice(L"PlaylistVisible",1,{0,1})!=0;
    state.equalizerVisible=choice(L"EqualizerVisible",0,{0,1})!=0;
    state.spectrum=choice(L"Spectrum",0,{0,1})!=0;
    state.scalePercent=choice(L"ScalePercent",0,{0,80,90,100,110,125,150});
    return state;
}
bool SaveAppearanceValue(const wchar_t* key,int value) {
    return path.empty() || WritePrivateProfileStringW(L"Appearance",key,std::to_wstring(value).c_str(),path.c_str());
}
void ThemeConfigure(const wchar_t* iniPath) {
    path=iniPath?iniPath:L"";
    wchar_t id[128]=L"original";
    if(!path.empty()) GetPrivateProfileStringW(L"Appearance",L"Theme",L"original",id,128,path.c_str());
    SelectTheme(id,false);
}
bool ThemeInitializeUser() {
    wchar_t folder[MAX_PATH]{};
    if(FAILED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,nullptr,SHGFP_TYPE_CURRENT,folder))) return false;
    std::wstring dir=std::wstring(folder)+L"\\LiteSic";
    if(!CreateDirectoryW(dir.c_str(),nullptr) && GetLastError()!=ERROR_ALREADY_EXISTS) return false;
    ThemeConfigure((dir+L"\\settings.ini").c_str());return true;
}
}
