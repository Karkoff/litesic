#include "runtime.h"
#include "ui_theme.h"
#include "ui_scale.h"
#include "m3u.h"
#include "check.h"
#include <string>
static std::wstring ini;
static void command_line_and_profile_contract(){
    Runtime::Options o;CHECK(Runtime::Parse({L"--portable",L"tests\\fixtures\\native-44100.wav"},o));CHECK(o.portable&&o.files.size()==1);CHECK(o.files[0]==FullPath(L"tests\\fixtures\\native-44100.wav"));
    CHECK(Runtime::ProfileDirectory(L"D:\\USB\\LiteSic.exe",L"C:\\User\\Local",o)==L"D:\\USB\\data");
    CHECK(Runtime::Parse({},o));CHECK(Runtime::ProfileDirectory(L"D:\\USB\\LiteSic.exe",L"C:\\User\\Local",o)==L"C:\\User\\Local\\LiteSic");
    CHECK(!Runtime::Parse({L"--unknown"},o));CHECK(!Runtime::Parse({L"--portable",L"--profile=x"},o));CHECK(!Runtime::Parse({L"--profile="},o));
    CHECK(Runtime::Identity(L"C:\\User\\LiteSic")==Runtime::Identity(L"c:/user/litesic"));CHECK(Runtime::Identity(L"c:/a")!=Runtime::Identity(L"c:/b"));
}
static void packaged_profile_is_separate_from_desktop_and_portable(){
    Runtime::Options options;
    const std::wstring local=L"C:\\User\\Local",exe=L"C:\\Program Files\\WindowsApps\\LiteSic\\LiteSic.exe";
    const std::wstring family=L"Brasiliense.LiteSic_testpublisher";
    auto packaged=Runtime::ProfileDirectory(exe,local,options,family);
    CHECK(packaged==local+L"\\Packages\\"+family+L"\\LocalState\\LiteSic");
    auto desktop=Runtime::ProfileDirectory(exe,local,options);
    CHECK(desktop==local+L"\\LiteSic");
    CHECK(Runtime::Identity(packaged)!=Runtime::Identity(desktop));
    CHECK(Runtime::Identity(packaged)!=Runtime::Identity(Runtime::ProfileDirectory(exe,local,options,L"LiteSic.LocalValidation_otherpublisher")));
    options.portable=true;CHECK(Runtime::ProfileDirectory(exe,local,options,family)==packaged);
    CHECK(Runtime::ProfileDirectory(L"D:\\USB\\LiteSic.exe",local,options)==L"D:\\USB\\data");
    CHECK(Runtime::ProfileDirectory(exe,L"",options,family).empty());
    options.portable=false;options.profile=L"build\\explicit-msix-test-profile";
    CHECK(Runtime::ProfileDirectory(exe,local,options,family)==FullPath(options.profile));
    std::wstring detected=L"must be cleared";CHECK(Runtime::CurrentPackageFamily(detected));
    CHECK(detected.empty()); // This unit-test executable is not packaged.
}
static void bounded_unicode_ipc(){
    std::vector<std::wstring> paths={L"C:\\音乐\\á.wav",L"D:\\a b\\x.flac"},decoded;
    auto packet=Runtime::Encode(paths);COPYDATASTRUCT data{Runtime::Packet,DWORD(packet.size()*2),packet.data()};
    CHECK(Runtime::Decode(&data,decoded));CHECK(decoded==paths);
    data.cbData=3;CHECK(!Runtime::Decode(&data,decoded));data.cbData=1024*1024+2;CHECK(!Runtime::Decode(&data,decoded));
    data.cbData=DWORD(packet.size()*2);data.dwData=0;CHECK(!Runtime::Decode(&data,decoded));data.dwData=Runtime::Packet;
    packet.back()=L'x';CHECK(!Runtime::Decode(&data,decoded));CHECK(!Runtime::Decode(nullptr,decoded));
    packet=Runtime::Encode({});data={Runtime::Packet,DWORD(packet.size()*2),packet.data()};CHECK(Runtime::Decode(&data,decoded)&&decoded.empty());
}
static void portable_marker_and_profile_precedence(){
    auto directory=FullPath(L"build\\portable-marker-音楽-"+std::to_wstring(sizeof(void*)*8)+L"-"+std::to_wstring(GetCurrentProcessId()));
    auto resources=directory+L"\\resources",marker=resources+L"\\portable.flag";
    CHECK(Runtime::PrepareProfile(resources));
    Runtime::Options options;auto exe=directory+L"\\LiteSic.exe";
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options)==L"C:\\Local\\LiteSic");
    CHECK(CreateDirectoryW(marker.c_str(),nullptr));
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options)==L"C:\\Local\\LiteSic");
    CHECK(RemoveDirectoryW(marker.c_str()));
    HANDLE handle=CreateFileW(marker.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    CHECK(handle!=INVALID_HANDLE_VALUE);CloseHandle(handle);
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options)==directory+L"\\data");
    CHECK(Runtime::ProfileDirectory(exe,L"",options)==directory+L"\\data");
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options,L"LiteSic_test")==L"C:\\Local\\Packages\\LiteSic_test\\LocalState\\LiteSic");
    options.profile=L"build\\explicit-portable-override";
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options)==FullPath(options.profile));
    options.profile.clear();options.validation=true;
    CHECK(Runtime::ProfileDirectory(exe,L"C:\\Local",options)!=directory+L"\\data");
    CHECK(DeleteFileW(marker.c_str()));CHECK(RemoveDirectoryW(resources.c_str()));CHECK(RemoveDirectoryW(directory.c_str()));
}
static void remaining_configuration_roundtrip(){
    DeleteFileW(ini.c_str());Ui::ThemeConfigure(ini.c_str());
    Ui::PanelPosition position;CHECK(!Ui::LoadPlayerPosition(position));CHECK(Ui::SavePlayerPosition({-900,-200}));CHECK(Ui::LoadPlayerPosition(position)&&position.left==-900&&position.top==-200);
    for(auto key:{L"Left",L"Top",L"Version"}){CHECK(WritePrivateProfileStringW(L"PlayerWindow",key,L"bad",ini.c_str()));CHECK(!Ui::LoadPlayerPosition(position));CHECK(Ui::SavePlayerPosition({-900,-200}));}

    for(int value:{0,1,500,999,1000}){CHECK(Ui::SaveAppearanceValue(L"PanPermille",value));Ui::ThemeConfigure(ini.c_str());CHECK(Ui::LoadAppearance().panPermille==value);}
    for(int repeat:{0,1,2}){CHECK(Ui::SaveAppearanceValue(L"Repeat",repeat));CHECK(Ui::LoadAppearance().repeat==repeat);}
    for(auto key:{L"Shuffle",L"Topmost",L"GlobalHotkeys"})for(int value:{0,1}){CHECK(Ui::SaveAppearanceValue(key,value));auto state=Ui::LoadAppearance();CHECK((!wcscmp(key,L"Shuffle")?state.shuffle:!wcscmp(key,L"Topmost")?state.topmost:state.globalHotkeys)==bool(value));}
    for(auto key:{L"PanPermille",L"Repeat",L"Shuffle",L"Topmost",L"GlobalHotkeys"})for(auto value:{L"bad",L"-1",L"99999999999999999999",L"1001"}){CHECK(WritePrivateProfileStringW(L"Appearance",key,value,ini.c_str()));auto state=Ui::LoadAppearance();if(!wcscmp(key,L"PanPermille"))CHECK(state.panPermille==500);else if(!wcscmp(key,L"Repeat"))CHECK(state.repeat==0);else CHECK(!(!wcscmp(key,L"Shuffle")?state.shuffle:!wcscmp(key,L"Topmost")?state.topmost:state.globalHotkeys));}
}
static void compact_display_preserves_geometry(){
    CHECK(Ui::FitScale(2,460,290,{0,0,0,0})==2);
    RECT work{-800,-200,0,400};for(float dpi:{1.f,1.25f,1.5f,2.f})for(float zoom:{1.f,1.25f,1.5f}){float scale=Ui::FitScale(dpi*zoom,460,290,work);CHECK(scale>0&&460*scale<=800.01f&&290*scale<=600.01f);}
}
static void explicit_profile_writability_and_readonly(){
    auto directory=FullPath(L"build\\m8-profile-音乐-"+std::to_wstring(sizeof(void*)*8));auto before=Runtime::Identity(directory);CHECK(Runtime::PrepareProfile(directory));CHECK(before==Runtime::Identity(directory));
    auto file=directory+L"\\settings.ini";HANDLE handle=CreateFileW(file.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);CHECK(handle!=INVALID_HANDLE_VALUE);CloseHandle(handle);
    CHECK(SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_READONLY));CHECK(!Runtime::PrepareProfile(directory));CHECK(SetFileAttributesW(file.c_str(),FILE_ATTRIBUTE_NORMAL));CHECK(DeleteFileW(file.c_str()));CHECK(RemoveDirectoryW(directory.c_str()));
}
int main(){ini=FullPath(L"build\\m8-config-"+std::to_wstring(sizeof(void*)*8)+L".ini");int cases=0;CASE(command_line_and_profile_contract);CASE(packaged_profile_is_separate_from_desktop_and_portable);CASE(portable_marker_and_profile_precedence);CASE(bounded_unicode_ipc);CASE(remaining_configuration_roundtrip);CASE(compact_display_preserves_geometry);CASE(explicit_profile_writability_and_readonly);Ui::ThemeConfigure(nullptr);DeleteFileW(ini.c_str());printf("%d remaining configuration cases\n",cases);}
