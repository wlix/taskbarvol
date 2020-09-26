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
HWND       g_hToolTipWnd       = nullptr;
ATOM       g_TaskSwitcherClass = NULL;
UINT_PTR   g_idTipTimer        = 0;
HANDLE     g_hSndVolProcess    = nullptr;

CONFIG_DATA  g_Config;

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
            break;*/

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            if (g_idTipTimer != 0) {
                KillTimer(NULL, g_idTipTimer);
            }
            close_sndvol_process();
            break;
            
        case WM_MOUSEWHEEL:
            switch (g_Config.indicator_type) {
            case INDICATOR_TYPE::TOOLTIP:
            case INDICATOR_TYPE::SNDVOL:
            case INDICATOR_TYPE::NONE:
                if ((signed short)HIWORD(msll->mouseData) > 0) {
                    adjust_master_volume(g_Config.bReverse ? -g_Config.difference : g_Config.difference);
                }
                else {
                    adjust_master_volume(g_Config.bReverse ? g_Config.difference : -g_Config.difference);
                }
                ShowVolume(msll);
                break;
            case INDICATOR_TYPE::KEYHACK:
                INPUT input;
                WORD  wVK;
                input.type = INPUT_KEYBOARD;
                if ((signed short)HIWORD(msll->mouseData) > 0) {
                    wVK = g_Config.bReverse ? VK_VOLUME_UP : VK_VOLUME_DOWN;
                }
                else {
                    wVK = g_Config.bReverse ? VK_VOLUME_DOWN : VK_VOLUME_UP;
                }
                input.ki = { wVK, 0 };
                SendInput(1, &input, sizeof(INPUT));
                input.ki = { wVK, 0, KEYEVENTF_KEYUP };
                SendInput(1, &input, sizeof(INPUT));
                break;
            }

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

    // コンフィグロード
    config::get_instance().load_config();

    // コモンコントロール初期化
    InitCommonControls();

    // タスクバーのハンドルを取得
    g_hTaskbarWnd = FindWindow(TEXT("Shell_TrayWnd"), NULL);
    if (g_hTaskbarWnd == NULL) {
        WriteLog(elError, TEXT("%s: taskbar is not found"), g_info.Name);
        return FALSE;
    }

    // ボリューム初期化
    volume_setting_init();

    // タスクスイッチャークラスを保存
    WNDCLASS wndclass;
    g_TaskSwitcherClass = (ATOM)GetClassInfo(GetModuleHandle(NULL), L"TaskSwitcherWnd", &wndclass);

    g_hToolTipWnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, g_hInst, NULL);
    SendMessage(g_hToolTipWnd, TTM_TRACKPOSITION, 0, MAKELPARAM(-500, -500));

    if (lstrcmp(g_Config.font_name, L"") != 0) {
        HDC hDC = GetDC(g_hToolTipWnd);
        int nHeight = -MulDiv(g_Config.font_height, GetDeviceCaps(hDC, LOGPIXELSY), 72);
        ReleaseDC(g_hToolTipWnd, hDC);
        g_Config.hFont = CreateFontW(nHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, g_Config.font_name);
        if (g_Config.hFont != NULL) {
            SendMessageW(g_hToolTipWnd, WM_SETFONT, (WPARAM)g_Config.hFont, 0);
        }
    }

    // マウスフック
    g_hHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, g_hInst, 0);
    if (g_hHook == NULL) { return FALSE; }

    PostMessage(g_hTaskbarWnd, WM_SIZE, SIZE_RESTORED, 0);

    return TRUE;
}

//---------------------------------------------------------------------------//

// TTBEvent_Unload() の内部実装
void WINAPI Unload(void) {
    // マウスアンフック
    ::UnhookWindowsHookEx(g_hHook);
    g_hHook = nullptr;

    if (g_hToolTipWnd != nullptr) {
        DestroyWindow(g_hToolTipWnd);
    }
    g_hToolTipWnd = nullptr;

    if (g_Config.hFont) {
        DeleteObject(g_Config.hFont);
    }
    g_Config.hFont = nullptr;

    // sndvol
    close_sndvol_process();

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
