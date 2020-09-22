#include <commctrl.h>
#include <shlwapi.h>
#include <sstream>

#include "display_volume.hpp"
#include "config.hpp"
#include "mixer.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")

extern CONFIG_DATA  g_Config;
extern HINSTANCE    g_hInst;
extern HWND         g_hTaskbarWnd;
extern HWND         g_hToolTipWnd;

BOOL bTooltipTimerOn = FALSE;

BOOL bSndVolModernLaunched = FALSE;
BOOL bSndVolModernAppeared = FALSE;
HWND hSndVolModernPreviousForegroundWnd = nullptr;

BOOL OpenScrollSndVol(WPARAM wParam, LPARAM lMousePosParam) {
	HANDLE hMutex;
	HWND hVolumeAppWnd;
	DWORD dwProcessId;
	WCHAR szCommandLine[sizeof("SndVol.exe -f 4294967295")];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (g_Config.indicator_type == INDICATOR_TYPE::TOOLTIP) {
		adjust_master_volume((float)(GET_WHEEL_DELTA_WPARAM(wParam) * (g_Config.difference / 120)));
		ShowSndVolTooltip();
		return TRUE;
	}
	else if (g_Config.indicator_type == INDICATOR_TYPE::MODERN && CanUseModernIndicator()) {
		adjust_master_volume((float)(GET_WHEEL_DELTA_WPARAM(wParam) * (g_Config.difference / 120)));
		ShowSndVolModernIndicator();
		return TRUE;
	}

	if (ValidateSndVolProcess()) {
		if (WaitForInputIdle(hSndVolProcess, 0) == 0) // If not initializing
		{
			if (ValidateSndVolWnd())
			{
				ScrollSndVol(wParam, lMousePosParam);

				return FALSE; // False because we didn't open it, it was open
			}
			else
			{
				hVolumeAppWnd = FindWindow(L"Windows Volume App Window", L"Windows Volume App Window");
				if (hVolumeAppWnd)
				{
					GetWindowThreadProcessId(hVolumeAppWnd, &dwProcessId);

					if (GetProcessId(hSndVolProcess) == dwProcessId)
					{
						BOOL bOpened;
						if (OpenScrollSndVolInternal(wParam, lMousePosParam, hVolumeAppWnd, &bOpened))
							return bOpened;
					}
				}
			}
		}

		return FALSE;
	}

	hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"Windows Volume App Window");
	if (hMutex)
	{
		CloseHandle(hMutex);

		hVolumeAppWnd = FindWindow(L"Windows Volume App Window", L"Windows Volume App Window");
		if (hVolumeAppWnd)
		{
			GetWindowThreadProcessId(hVolumeAppWnd, &dwProcessId);

			hSndVolProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, dwProcessId);
			if (hSndVolProcess)
			{
				if (WaitForInputIdle(hSndVolProcess, 0) == 0) // if not initializing
				{
					if (ValidateSndVolWnd())
					{
						ScrollSndVol(wParam, lMousePosParam);

						return FALSE; // False because we didn't open it, it was open
					}
					else
					{
						BOOL bOpened;
						if (OpenScrollSndVolInternal(wParam, lMousePosParam, hVolumeAppWnd, &bOpened))
							return bOpened;
					}
				}
			}
		}

		return FALSE;
	}

	wsprintf(szCommandLine, L"SndVol.exe -f %u", (DWORD)lMousePosParam);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if (!CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, ABOVE_NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, NULL, NULL, &si, &pi))
		return FALSE;

	AllowSetForegroundWindow(pi.dwProcessId);
	ResumeThread(pi.hThread);

	CloseHandle(pi.hThread);
	hSndVolProcess = pi.hProcess;

	return TRUE;
}

// --------------------------------------------------------
//  ToolTip indicator
// --------------------------------------------------------
void OnSndVolTooltipTimer()
{
	HWND hTrayToolbarWnd;
	int index;
	HWND hTooltipWnd;

	if (!bTooltipTimerOn)
		return;

	bTooltipTimerOn = FALSE;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0)
		return;

	hTooltipWnd = (HWND)SendMessage(hTrayToolbarWnd, TB_GETTOOLTIPS, 0, 0);
	if (hTooltipWnd)
		ShowWindow(hTooltipWnd, SW_HIDE);
}

BOOL ShowSndVolTooltip() {
	HWND hTrayToolbarWnd;
	int index;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0)
		return FALSE;

	SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, -1, 0);
	SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, index, 0);

	// Show tooltip
	bTooltipTimerOn = TRUE;
	SetTimer(hTrayToolbarWnd, 0, 0, NULL);

	return TRUE;
}

BOOL HideSndVolTooltip()
{
	HWND hTrayToolbarWnd;
	int index;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0)
		return FALSE;

	if (SendMessage(hTrayToolbarWnd, TB_GETHOTITEM, 0, 0) == index)
		SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, -1, 0);

	return TRUE;
}

int GetSndVolTrayIconIndex(HWND* phTrayToolbarWnd)
{
	RECT rcSndVolIcon;
	HWND hTrayNotifyWnd, hTrayToolbarWnd;
	LONG_PTR lpTrayNotifyLongPtr;
	POINT pt;
	int index;

	index = -1;

	if (GetSndVolTrayIconRect(&rcSndVolIcon))
	{
		hTrayNotifyWnd = *EV_TASKBAR_TRAY_NOTIFY_WND;
		if (hTrayNotifyWnd)
		{
			lpTrayNotifyLongPtr = GetWindowLongPtr(hTrayNotifyWnd, 0);

			hTrayToolbarWnd = *EV_TRAY_NOTIFY_TOOLBAR_WND(lpTrayNotifyLongPtr);
			if (hTrayToolbarWnd)
			{
				pt.x = rcSndVolIcon.left + (rcSndVolIcon.right - rcSndVolIcon.left) / 2;
				pt.y = rcSndVolIcon.top + (rcSndVolIcon.bottom - rcSndVolIcon.top) / 2;

				MapWindowPoints(NULL, hTrayToolbarWnd, &pt, 1);

				index = (int)SendMessage(hTrayToolbarWnd, TB_HITTEST, 0, (LPARAM)&pt);
				if (index >= 0 && phTrayToolbarWnd)
					*phTrayToolbarWnd = hTrayToolbarWnd;
			}
		}
	}

	return index;
}

// --------------------------------------------------------
//  Modern indicator
// --------------------------------------------------------
BOOL CanUseModernIndicator() {
	DWORD dwEnabled = 1;
	DWORD dwValueSize = sizeof(dwEnabled);
	DWORD dwError = RegGetValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\MTCUVC",
		L"EnableMTCUVC", RRF_RT_REG_DWORD, NULL, &dwEnabled, &dwValueSize);

	return dwEnabled != 0;
}

BOOL ShowSndVolModernIndicator() {
	if (bSndVolModernLaunched) {
		return TRUE; // already launched
	}

	HWND hSndVolModernIndicatorWnd = GetOpenSndVolModernIndicatorWnd();
	if (hSndVolModernIndicatorWnd) {
		return TRUE; // already shown
	}

	HWND hForegroundWnd = GetForegroundWindow();
	if (hForegroundWnd && hForegroundWnd != g_hTaskbarWnd)
		hSndVolModernPreviousForegroundWnd = hForegroundWnd;

	HWND hSndVolTrayControlWnd = GetSndVolTrayControlWnd();
	if (!hSndVolTrayControlWnd)
		return FALSE;

	if (!PostMessage(hSndVolTrayControlWnd, 0x460, 0, MAKELPARAM(NIN_SELECT, 100)))
		return FALSE;

	bSndVolModernLaunched = TRUE;
	return TRUE;
}

BOOL HideSndVolModernIndicator() {
	HWND hSndVolModernIndicatorWnd = GetOpenSndVolModernIndicatorWnd();
	if (hSndVolModernIndicatorWnd) {
		if (!hSndVolModernPreviousForegroundWnd || !SetForegroundWindow(hSndVolModernPreviousForegroundWnd)) {
			SetForegroundWindow(g_hTaskbarWnd);
		}
	}
	return TRUE;
}

void EndSndVolModernIndicatorSession() {
	hSndVolModernPreviousForegroundWnd = nullptr;
	bSndVolModernLaunched = FALSE;
	bSndVolModernAppeared = FALSE;
}

BOOL IsCursorUnderSndVolModernIndicatorWnd() {
	HWND hSndVolModernIndicatorWnd = GetOpenSndVolModernIndicatorWnd();
	if(!hSndVolModernIndicatorWnd) { return FALSE; }

	POINT pt;
	GetCursorPos(&pt);
	return WindowFromPoint(pt) == hSndVolModernIndicatorWnd;
}

HWND GetOpenSndVolModernIndicatorWnd() {
	HWND hForegroundWnd = GetForegroundWindow();
	if (!hForegroundWnd)
		return NULL;

	// Check class name
	WCHAR szBuffer[32];
	if (!GetClassName(hForegroundWnd, szBuffer, 32) ||
		wcscmp(szBuffer, L"Windows.UI.Core.CoreWindow") != 0)
		return NULL;

	// Check that the MtcUvc prop exists
	WCHAR szVerifyPropName[sizeof("ApplicationView_CustomWindowTitle#1234567890#MtcUvc")];
	wsprintf(szVerifyPropName, L"ApplicationView_CustomWindowTitle#%u#MtcUvc", (DWORD)(DWORD_PTR)hForegroundWnd);

	SetLastError(0);
	GetProp(hForegroundWnd, szVerifyPropName);
	if (GetLastError() != 0)
		return NULL;

	return hForegroundWnd;
}

HWND GetSndVolTrayControlWnd() {
	HWND hBluetoothNotificationWnd = FindWindow(L"BluetoothNotificationAreaIconWindowClass", NULL);
	if(!hBluetoothNotificationWnd)
		return NULL;

	HWND hWnd = NULL;
	EnumThreadWindows(GetWindowThreadProcessId(hBluetoothNotificationWnd, NULL), EnumThreadFindSndVolTrayControlWnd, (LPARAM)&hWnd);
	return hWnd;
}

BOOL CALLBACK EnumThreadFindSndVolTrayControlWnd(HWND hWnd, LPARAM lParam) {
	HMODULE hInstance = (HMODULE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
	if (hInstance && hInstance == GetModuleHandle(L"sndvolsso.dll"))
	{
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}

// --------------------------------------------------------
//  ツールチップ関連
// --------------------------------------------------------
// ツールチップ消去用のタイマー
VOID CALLBACK TimerProc_Tip(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	SendMessage(g_hToolTipWnd, TTM_TRACKACTIVATE, (WPARAM)FALSE, NULL);
	KillTimer(NULL, g_Config.id_timer);
	g_config.id_tip_timer = 0;
}

// ツールチップを表示
BOOL ShowToolTip(const wchar_t* szText, const wchar_t* szTitle)
{
	TTTOOLINFOW ti = { 0 };
	RECT rc;

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
	ti.lpszText = (LPWSTR)szText;

	if (szTitle)
		SendMessageW(g_hToolTipWnd, TTM_SETTITLE, (WPARAM)TTI_INFO, (LPARAM)szTitle);
	SendMessageW(g_hToolTipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
	SendMessageW(g_hToolTipWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);

	SendMessageW(g_hToolTipWnd, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);

	GetWindowRect(g_hToolTipWnd, &rc);
	SendMessageW(g_hToolTipWnd, TTM_TRACKPOSITION, 0,
		MAKELPARAM((GetSystemMetrics(SM_CXSCREEN) - (rc.right - rc.left)) * g_Config.display_position.x / 100,
			(GetSystemMetrics(SM_CYSCREEN) - (rc.bottom - rc.top)) * g_Config.display_position.y / 100));

	SetWindowPos(g_hToolTipWnd, HWND_TOPMOST, 0, 0, 0, 0,
		SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	if (g_Config.id_timer != 0)
		KillTimer(NULL, g_Config.id_timer);
	g_Config.id_timer = SetTimer(NULL, 0, g_Config.display_time, TimerProc_Tip);
	return TRUE;
}


// Snc.dllで表示
void ShowSnc(LPCWSTR title, LPCWSTR text)
{
	wchar_t Buffer[512 + 512 + MAX_PATH + 64] = L"";
	wchar_t wTitle[512] = L"", wText[512] = L"", wTimeout[32] = L"", wIcon[MAX_PATH] = L"";

	if (title != NULL)
		lstrcpyW(wTitle, title);
	lstrcpyW(wText, text);
	wsprintfW(wTimeout, L"%d", g_Config.display_time / 1000);

	GetModuleFileNameW(g_hInst, wIcon, MAX_PATH);
	PathRenameExtensionW(wIcon, L".ico");

	lstrcatW(Buffer, L"\"/title=");
	lstrcatW(Buffer, wTitle);
	lstrcatW(Buffer, L"\" ");

	lstrcatW(Buffer, L"\"/text=");
	lstrcatW(Buffer, wText);
	lstrcatW(Buffer, L"\" ");

	lstrcatW(Buffer, L"/timeout=");
	lstrcatW(Buffer, wTimeout);
	lstrcatW(Buffer, L" ");

	if (PathFileExistsW(wIcon))
	{
		lstrcatW(Buffer, L"\"/icon=");
		lstrcatW(Buffer, wIcon);
		lstrcatW(Buffer, L"\"");
	}

	COPYDATASTRUCT cd;
	cd.dwData = 0x100;
	cd.cbData = (lstrlenW(Buffer) + 1) * sizeof(wchar_t);
	cd.lpData = Buffer;

	HWND hwnd = FindWindowW(NULL, L"Snarl");
	if (hwnd != NULL)
		SendMessage(hwnd, WM_COPYDATA, 0, (LPARAM)&cd);
}

// --------------------------------------------------------
//    ボリュームを表示する
// --------------------------------------------------------
// フォーマット
BOOL FormatText(const FormatItem& format, std::wstring& wstr) {
	std::wstringstream wss;

	switch (format.type) {
	case FormatItem::STRING:
		wss << format.str;
		break;
	case FormatItem::VOLUME:
		BOOL mute;
		get_mute_state(&mute);
		if (mute) {
			wss << L"Mute";
		}
		else {
			float volume = .0f;
			get_master_volume(&volume);
			wss << (volume * 100.0f) << L"%";
		}
		break;
	}

	wstr = wss.str();
	return TRUE;
}
