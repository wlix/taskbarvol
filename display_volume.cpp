#include "display_volume.hpp"
#include "config.hpp"
#include "mixer.hpp"

#include <shlwapi.h>
#include <commctrl.h>

#include <iomanip>
#include <sstream>

extern CONFIG_DATA g_Config;
extern HINSTANCE   g_hInst;
extern HWND        g_hToolTipWnd;
extern UINT_PTR    g_idTipTimer;
extern HANDLE      g_hSndVolProcess;

void CALLBACK TimerProc_Tip(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    SendMessage(g_hToolTipWnd, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
    KillTimer(NULL, g_idTipTimer);
    g_idTipTimer = 0;

    close_sndvol_process();
}

BOOL ShowToolTip(const wchar_t* szText) {
    TTTOOLINFOW ti = { 0 };
    RECT rc;

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
    ti.lpszText = (LPWSTR)szText;

    SendMessageW(g_hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
    SendMessageW(g_hToolTipWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

    SendMessageW(g_hToolTipWnd, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);

    GetWindowRect(g_hToolTipWnd, &rc);
    SendMessageW(g_hToolTipWnd, TTM_TRACKPOSITION, 0,
        MAKELPARAM((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) * g_Config.display_position.x / 100,
        (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) * g_Config.display_position.y / 100));

    SetWindowPos(g_hToolTipWnd, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    if (g_idTipTimer != 0) {
        KillTimer(NULL, g_idTipTimer);
    }
    g_idTipTimer = SetTimer(NULL, 0, g_Config.display_time, TimerProc_Tip);

    return TRUE;
}

BOOL ShowSndVol(LPARAM lParam) {
    WCHAR szCommandLine[sizeof("SndVol.exe -f 4294967295")];
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    wsprintf(szCommandLine, L"SndVol.exe -f %u", (DWORD)lParam);

    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, ABOVE_NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, NULL, NULL, &si, &pi))
        return FALSE;

    AllowSetForegroundWindow(pi.dwProcessId);
    ResumeThread(pi.hThread);

    CloseHandle(pi.hThread);
    g_hSndVolProcess = pi.hProcess;

    if (g_idTipTimer != 0) {
        KillTimer(NULL, g_idTipTimer);
    }
    g_idTipTimer = SetTimer(NULL, 0, g_Config.display_time, TimerProc_Tip);

    return TRUE;
}

void close_sndvol_process() {
    if (g_hSndVolProcess) {
        CloseHandle(g_hSndVolProcess);
        g_hSndVolProcess = nullptr;
    }
}

BOOL ShowVolume(LPMSLLHOOKSTRUCT msll) {
    switch (g_Config.indicator_type) {
    case INDICATOR_TYPE::TOOLTIP:
        {
            std::wstringstream wss;
            BOOL mute;
            get_mute_state(&mute);
            if (mute) {
                wss << L"Mute";
            }
            else {
                float volume = .0f;
                get_master_volume(&volume);
                wss << L"Volume: " << std::setw(3) << (int)(volume * 100) << L"%";
            }

            ShowToolTip(wss.str().c_str());
        }
        break;
    case INDICATOR_TYPE::SNDVOL:
        {
            if (!g_hSndVolProcess) {
                ShowSndVol(MAKELPARAM(msll->pt.x, msll->pt.y));
            }
        }
        break;
    case INDICATOR_TYPE::KEYHACK:
        break;
    case INDICATOR_TYPE::NONE:
        break;
    }

    return TRUE;
}
