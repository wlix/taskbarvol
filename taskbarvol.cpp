//---------------------------------------------------------------------------//
//
// Main.cpp
//
//---------------------------------------------------------------------------//

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "mixer.hpp"


//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

#pragma data_seg(".SHARED_DATA")
HHOOK  g_hHook  { nullptr };
#pragma data_seg()

HINSTANCE    g_hInst  { nullptr };
HANDLE       g_hMutex{ nullptr };

HWND	g_hTaskWnd = NULL;
BOOL	g_bReverse = FALSE;
int		g_dif = 0;
int     g_MuteKey = 0;

// プラグインの名前
#if defined(WIN64) || defined(_WIN64)
LPWSTR PLUGIN_NAME  { L"TaskBarVol for Win10 x64" };
#else
LPSTR PLUGIN_NAME   {  "TaskBarVol for Win10 x86" };
#endif

// コマンドの数
DWORD COMMAND_COUNT { 0 };

// プラグインの情報
PLUGIN_INFO g_info = {
    0,                   // プラグインI/F要求バージョン
    PLUGIN_NAME,         // プラグインの名前（任意の文字が使用可能）
    nullptr,             // プラグインのファイル名（相対パス）
    ptAlwaysLoad,        // プラグインのタイプ
    0,                   // バージョン
    0,                   // バージョン
    COMMAND_COUNT,       // コマンド個数
    NULL,                // コマンド
    0,                   // ロードにかかった時間（msec）
};

// --------------------------------------------------------
//    フックプロシージャ
// --------------------------------------------------------
LRESULT CALLBACK LowLevelMouseProc(_In_ int nCode, _In_ WPARAM wParam, _In_ LPARAM lParam) {
    if (nCode == HC_ACTION) {
        LPMSLLHOOKSTRUCT msll = (LPMSLLHOOKSTRUCT)lParam;
        HWND hMouseWnd = WindowFromPoint(msll->pt);
        if (GetAncestor(hMouseWnd, GA_ROOT) != g_hTaskWnd) {
            return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
        }
        if (wParam == WM_MOUSEWHEEL) {
            if ((signed short)HIWORD(msll->mouseData) > 0) {
                adjust_master_volume(g_bReverse ? -g_dif : g_dif);
            } else {
                adjust_master_volume(g_bReverse ? g_dif : -g_dif);
            }
            return TRUE;
        }
    }
    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// TTBEvent_Init() の内部実装
BOOL WINAPI Init(void) {
    // フックのために二重起動を禁止
    if (g_hMutex == nullptr)
    {
        g_hMutex = ::CreateMutex(nullptr, TRUE, PLUGIN_NAME);
    }
    if (g_hMutex == nullptr)
    {
        WriteLog(elError, TEXT("%s: Failed to create mutex"), g_info.Name);
        return FALSE;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS)
    {
        WriteLog(elError, TEXT("%s: %s is already started"), g_info.Name, g_info.Name);
        return FALSE;
    }

    TCHAR iniPath[MAX_PATH];
    int len = GetModuleFileName(g_hInst, iniPath, MAX_PATH);

    iniPath[len - 3] = 'i';
    iniPath[len - 2] = 'n';
    iniPath[len - 1] = 'i';

    g_bReverse = GetPrivateProfileInt(TEXT("Setting"), TEXT("Reverse"), 0, iniPath);
    g_dif = GetPrivateProfileInt(TEXT("Setting"), TEXT("Diff"), 1, iniPath);

    g_hTaskWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (g_hTaskWnd == NULL) {
        WriteLog(elError, TEXT("FindWindow is Failed."));
        return FALSE;
    }

    volume_setting_init();

    g_hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, g_hInst, 0);
    if (g_hHook == NULL)
        return FALSE;

    PostMessage(g_hTaskWnd, WM_SIZE, SIZE_RESTORED, 0);

    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload(void) {
    ::UnhookWindowsHookEx(g_hHook);
    g_hHook = NULL;

    volume_setting_deinit();

    // ミューテックスを削除
    if (g_hMutex != nullptr)
    {
        ::ReleaseMutex(g_hMutex);
        ::CloseHandle(g_hMutex);
        g_hMutex = nullptr;
    }

    WriteLog(elInfo, TEXT("%s: successfully uninitialized"), g_info.Name);
}

//---------------------------------------------------------------------------//

// TTBEvent_Execute() の内部実装
BOOL WINAPI Execute(INT32 CmdId, HWND hWnd) {
    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_WindowsHook() の内部実装
void WINAPI Hook(UINT Msg, WPARAM wParam, LPARAM lParam) {
}

//---------------------------------------------------------------------------//
//
// CRT を使わないため new/delete を自前で実装
//
//---------------------------------------------------------------------------//

#if defined(_NODEFLIB)

void* __cdecl operator new(size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete(void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete(void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void* __cdecl operator new[](size_t size)
{
    return ::HeapAlloc(::GetProcessHeap(), 0, size);
}

void __cdecl operator delete[](void* p)
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

void __cdecl operator delete[](void* p, size_t) // C++14
{
    if ( p != nullptr ) ::HeapFree(::GetProcessHeap(), 0, p);
}

// プログラムサイズを小さくするためにCRTを除外
#pragma comment(linker, "/nodefaultlib:libcmt.lib")
#pragma comment(linker, "/entry:DllMain")

#endif

//---------------------------------------------------------------------------//

// DLL エントリポイント
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        g_hInst = hInstance;
    }
    return TRUE;
}
