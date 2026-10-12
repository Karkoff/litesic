#include "runtime.h"
#include <shellapi.h>
#include "i18n.h"
#include "language_selection.h"
/*
 * app.cpp — Entry point (wWinMain) + loop de mensagens
 *
 * M1-T1: registra a classe, cria a janela principal, executa o loop
 * GetMessageW/TranslateMessage/DispatchMessageW.
 * M8-T3: Per-Monitor DPI Awareness V2 para telas 4K/high-DPI.
 */

#include <windows.h>
#include <shlobj.h>
#include <string>

#include "mainwnd.h"
#include "plwnd.h"
#include "eqwnd.h"
#include "ui_theme.h"

/* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 requer Windows 10 1703+.
 * Verificar existência da função antes de chamar (fallback para o manifest). */
typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);

static void InitDpiAwareness(void)
{
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32 != NULL)
    {
        PFN_SetProcessDpiAwarenessContext pSetDpi =
            (PFN_SetProcessDpiAwarenessContext)GetProcAddress(
                hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi != NULL)
        {
            /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = -4 */
            pSetDpi((HANDLE)-4);
        }
        FreeLibrary(hUser32);
    }
}

/* ------------------------------------------------------------------ */
/* Entry point                                                          */
/* ------------------------------------------------------------------ */

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/,
                    LPWSTR /*lpCmdLine*/, int nCmdShow)
{
    /* Configurar Per-Monitor DPI Awareness V2 antes de criar janelas. */
    InitDpiAwareness();
    std::string catalogError;
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    std::vector<std::wstring> args;for(int i=1;i<argc;++i)args.emplace_back(argv[i]);if(argv)LocalFree(argv);
    Runtime::Options options;bool parsed=argc>0&&Runtime::Parse(args,options);
    wchar_t executable[32768]{},localData[MAX_PATH]{};GetModuleFileNameW(nullptr,executable,32768);
    SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,localData);
    auto directory=Runtime::ProfileDirectory(executable,localData,options);
    Runtime::Instance instance;
    int election=parsed?instance.Enter(directory,options.files):-1;
    if(election==0){if(SUCCEEDED(com))CoUninitialize();return 0;}
    bool profileReady=election==1&&Runtime::PrepareProfile(directory);
    Ui::ThemeConfigure(profileReady?(directory+L"\\settings.ini").c_str():nullptr);
    I18n::InitializeLanguage(Ui::SettingsPath());
    if(!profileReady){MessageBoxW(nullptr,I18n::Text(election<0?"errors.instance":"errors.data_folder").c_str(),L"LiteSic",MB_OK|MB_ICONERROR);if(SUCCEEDED(com))CoUninitialize();return 1;}
    if(options.validation&&!options.locale.empty()){
        if(!I18n::Select(std::string(options.locale.begin(),options.locale.end()),catalogError))OutputDebugStringA(catalogError.c_str());
    }
    /* Registrar a classe da janela (uma vez). */
    if (!MainWnd_RegisterClass(hInstance))
    {
        if (SUCCEEDED(com)) CoUninitialize();
        return 1;
    }

    /* Criar e exibir a janela principal. */
    HWND hwnd = MainWnd_Create(hInstance, nCmdShow);
    if (hwnd == NULL)
    {
        if (SUCCEEDED(com)) CoUninitialize();
        return 1;
    }
    std::wstring session=directory+L"\\playlist.m3u8";
    if (!PlWnd_Create(hInstance, hwnd, session.c_str())) {
        DestroyWindow(hwnd);
        if (SUCCEEDED(com)) CoUninitialize();
        return 1;
    }

    EqWnd_Restore(hwnd);
    MainWnd_ApplyTopmost(hwnd);
    instance.Publish(hwnd);
    if(!PlWnd_AppendPaths(options.files))MessageBoxW(hwnd,I18n::Text("errors.play_file").c_str(),L"LiteSic",MB_OK|MB_ICONERROR);

    /* Loop de mensagens padrão. */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if(MainWnd_TranslateShortcut(&msg)) continue;
        if(Ui::ThemeDialogMessage(&msg)) continue;
        if(PlWnd_DialogMessage(&msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (SUCCEEDED(com)) CoUninitialize();
    return (int)msg.wParam;
}
