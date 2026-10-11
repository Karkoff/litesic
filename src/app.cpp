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
                    LPWSTR lpCmdLine, int nCmdShow)
{
    /* Configurar Per-Monitor DPI Awareness V2 antes de criar janelas. */
    InitDpiAwareness();
    HRESULT com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    std::wstring validationDirectory;
    // Fixed, executable-relative developer session. Never touches the personal INI/playlist.
    if(!wcscmp(lpCmdLine,L"--validation")) {
        wchar_t executable[MAX_PATH]{};GetModuleFileNameW(nullptr,executable,MAX_PATH);
        validationDirectory=executable;
        for(int i=0;i<4;++i)validationDirectory=validationDirectory.substr(0,validationDirectory.find_last_of(L"\\/"));
        validationDirectory+=L"\\build\\gui";CreateDirectoryW(validationDirectory.c_str(),nullptr);
        validationDirectory+=L"\\m6-session";CreateDirectoryW(validationDirectory.c_str(),nullptr);
        Ui::ThemeConfigure((validationDirectory+L"\\settings.ini").c_str());
    } else Ui::ThemeInitializeUser();

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
    wchar_t appData[MAX_PATH]{};
    std::wstring session;
    if(!validationDirectory.empty()) session=validationDirectory+L"\\playlist.m3u8";
    else if (SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA|CSIDL_FLAG_CREATE,
                                  nullptr,SHGFP_TYPE_CURRENT,appData))) {
        std::wstring directory=std::wstring(appData)+L"\\LiteSic";
        if (CreateDirectoryW(directory.c_str(),nullptr) || GetLastError()==ERROR_ALREADY_EXISTS)
            session=directory+L"\\playlist.m3u8";
    }
    if (session.empty()) MessageBoxW(hwnd,L"Não foi possível acessar a pasta de dados. A playlist não será preservada nesta sessão.",
                                   L"LiteSic",MB_OK|MB_ICONWARNING);
    if (!PlWnd_Create(hInstance, hwnd, session.c_str())) {
        DestroyWindow(hwnd);
        if (SUCCEEDED(com)) CoUninitialize();
        return 1;
    }

    EqWnd_Restore(hwnd);

    /* Loop de mensagens padrão. */
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0)
    {
        if(Ui::ThemeDialogMessage(&msg)) continue;
        if(PlWnd_DialogMessage(&msg)) continue;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (SUCCEEDED(com)) CoUninitialize();
    return (int)msg.wParam;
}
