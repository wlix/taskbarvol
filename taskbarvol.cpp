//---------------------------------------------------------------------------//
//
// Main.cpp
//
//---------------------------------------------------------------------------//

#include "Plugin.hpp"
#include "MessageDef.hpp"
#include "Utility.hpp"

#include "config.hpp"
#include "mixer.hpp"
#include "display_volume.hpp"

#include <commctrl.h>

VS_FIXEDFILEINFO* GetModuleVersionInfo(HMODULE hModule, UINT* puPtrLen) {
    HRSRC hResource;
    HGLOBAL hGlobal;
    void *pData, *pFixedFileInfo;
    UINT uPtrLen;

    pFixedFileInfo = NULL;
    uPtrLen = 0;

    if ((hResource = FindResource(hModule, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION)) != NULL) {
        if ((hGlobal = LoadResource(hModule, hResource)) != NULL) {
            if ((pData = LockResource(hGlobal)) != NULL) {
                if (!VerQueryValue(pData, L"\\", &pFixedFileInfo, &uPtrLen) || uPtrLen == 0) {
                    pFixedFileInfo = NULL;
                    uPtrLen = 0;
                }
            }
        }
    }

    return (VS_FIXEDFILEINFO*)pFixedFileInfo;
}

//---------------------------------------------------------------------------//
//
// グローバル変数
//
//---------------------------------------------------------------------------//

#pragma data_seg(".SHARED_DATA")
HHOOK      g_hHook  { nullptr };
#pragma data_seg()

HINSTANCE  g_hInst  { nullptr };
HANDLE     g_hMutex { nullptr };

HWND	   g_hTaskbarWnd       = nullptr;
LONG_PTR   lpTaskbarLongPtr;
HWND       g_hToolTipWnd       = nullptr;
ATOM       g_TaskSwitcherClass = NULL;

CONFIG_DATA  g_Config;

int  nWinVersion = WIN_VERSION_UNSUPPORTED;
WORD nExplorerBuild, nExplorerQFE;

// プラグインの名前
#if defined(WIN64) || defined(_WIN64)
LPWSTR PLUGIN_NAME  { L"TaskBarVol for Win10 x64" };
#else
LPSTR  PLUGIN_NAME  {  "TaskBarVol for Win10 x86" };
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

        if (g_TaskSwitcherClass) {
            HWND hForegroundWnd = GetForegroundWindow();
            if (hForegroundWnd) {
                ATOM wClassAtom = (ATOM)GetClassLong(hForegroundWnd, GCW_ATOM);
                if (wClassAtom && wClassAtom == g_TaskSwitcherClass) {
                    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
                }
            }
        }

        HWND hMouseWnd = WindowFromPoint(msll->pt);
        if (GetAncestor(hMouseWnd, GA_ROOT) != g_hTaskbarWnd) {
            return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
        }
        switch (wParam) {
        /*case WM_MOUSEMOVE:
            OnSndVolMouseMove(msll->pt);
            break;

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            OnSndVolMouseClick();
            break;
            */
        case WM_MOUSEWHEEL:
            adjust_master_volume(g_Config.bReverse ?  (float)(GET_WHEEL_DELTA_WPARAM(wParam) * (g_Config.difference / 120))
                                                   : -(float)(GET_WHEEL_DELTA_WPARAM(wParam) * (g_Config.difference / 120)) );
            ShowSndVolTooltip();
            return TRUE;
        }
    }
    return ::CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// TTBEvent_Init() の内部実装
BOOL WINAPI Init(void) {
    // フックのために二重起動を禁止
    if (g_hMutex == nullptr) {
        g_hMutex = ::CreateMutex(nullptr, TRUE, PLUGIN_NAME);
    }
    if (g_hMutex == nullptr) {
        WriteLog(elError, TEXT("%s: Failed to create mutex"), g_info.Name);
        return FALSE;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        WriteLog(elError, TEXT("%s: %s is already started"), g_info.Name, g_info.Name);
        return FALSE;
    }

    // OSバージョンのチェック
    VS_FIXEDFILEINFO* pFixedFileInfo = GetModuleVersionInfo(NULL, NULL);
    if (pFixedFileInfo) {
        switch (HIWORD(pFixedFileInfo->dwFileVersionMS)) { // Major version
        case 6:
            switch (LOWORD(pFixedFileInfo->dwFileVersionMS)) { // Minor version
            case 1:
                nWinVersion = WIN_VERSION_7;
                break;
            case 2:
                nWinVersion = WIN_VERSION_8;
                break;
            case 3:
                if (LOWORD(pFixedFileInfo->dwFileVersionLS) < 17000) { nWinVersion = WIN_VERSION_81; }
                else { nWinVersion = WIN_VERSION_811; };
                break;
            case 4:
                nWinVersion = WIN_VERSION_10_T1;
                break;
            }
            break;
        case 10:
            if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 10240)       nWinVersion = WIN_VERSION_10_T1;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 10586)  nWinVersion = WIN_VERSION_10_T2;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 14393)  nWinVersion = WIN_VERSION_10_R1;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 15063)  nWinVersion = WIN_VERSION_10_R2;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 16299)  nWinVersion = WIN_VERSION_10_R3;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 17134)  nWinVersion = WIN_VERSION_10_R4;
            else if (HIWORD(pFixedFileInfo->dwFileVersionLS) <= 17763)  nWinVersion = WIN_VERSION_10_R5;
            else                                                        nWinVersion = WIN_VERSION_10_19H1;
            break;
        }
        nExplorerBuild = HIWORD(pFixedFileInfo->dwFileVersionLS);
        nExplorerQFE = LOWORD(pFixedFileInfo->dwFileVersionLS);
    }

    // コンフィグロード
    config::get_instance().load_config();

    // タスクバーのハンドルを取得
    g_hTaskbarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (g_hTaskbarWnd == NULL) {
        WriteLog(elError, TEXT("%s: taskbar is not found"), g_info.Name);
        return FALSE;
    }
    lpTaskbarLongPtr = GetWindowLongPtr(g_hTaskbarWnd, 0);

    // ボリューム初期化
    volume_setting_init();

    // タスクスイッチャークラスを保存
    WNDCLASS wndclass;
    g_TaskSwitcherClass = (ATOM)GetClassInfo(GetModuleHandle(NULL), L"TaskSwitcherWnd", &wndclass);

    // マウスフック
    g_hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, g_hInst, 0);
    if (g_hHook == NULL)
        return FALSE;

    PostMessage(g_hTaskbarWnd, WM_SIZE, SIZE_RESTORED, 0);

    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload(void) {
    // マウスアンフック
    ::UnhookWindowsHookEx(g_hHook);
    g_hHook = nullptr;

    // ボリューム終了処理
    volume_setting_deinit();

    // ミューテックスを削除
    if (g_hMutex != nullptr) {
        ::ReleaseMutex(g_hMutex);
        ::CloseHandle(g_hMutex);
        g_hMutex = nullptr;
    }

    g_TaskSwitcherClass = NULL;
    g_hTaskbarWnd = nullptr;

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
