#include "display_volume.hpp"
#include "config.hpp"
#include "mixer.hpp"

#include <commctrl.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <sstream>

extern CONFIG_DATA  g_Config;
extern HINSTANCE    g_hInst;
extern HWND         g_hTaskbarWnd;
extern HWND         g_hToolTipWnd;

HANDLE        hSndVolProcess                     = nullptr;
HWND          hSndVolWnd                         = nullptr;
UINT_PTR      nCloseSndVolTimer                  = NULL;
int           nCloseSndVolTimerCount             = 0;
volatile BOOL bCloseOnMouseLeave                 = FALSE;
BOOL          bTooltipTimerOn                    = FALSE;
BOOL          bSndVolModernLaunched              = FALSE;
BOOL          bSndVolModernAppeared              = FALSE;
HWND          hSndVolModernPreviousForegroundWnd = nullptr;

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
		if (WaitForInputIdle(hSndVolProcess, 0) == 0) { // If not initializing
			if (ValidateSndVolWnd()) {
				ScrollSndVol(wParam, lMousePosParam);
				return FALSE; // False because we didn't open it, it was open
			}
			else {
				hVolumeAppWnd = FindWindow(L"Windows Volume App Window", L"Windows Volume App Window");
				if (hVolumeAppWnd) {
					GetWindowThreadProcessId(hVolumeAppWnd, &dwProcessId);

					if (GetProcessId(hSndVolProcess) == dwProcessId) {
						BOOL bOpened;
						if (OpenScrollSndVolInternal(wParam, lMousePosParam, hVolumeAppWnd, &bOpened)) {
							return bOpened;
						}
					}
				}
			}
		}
		return FALSE;
	}

	hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"Windows Volume App Window");
	if (hMutex) {
		CloseHandle(hMutex);

		hVolumeAppWnd = FindWindow(L"Windows Volume App Window", L"Windows Volume App Window");
		if (hVolumeAppWnd) {
			GetWindowThreadProcessId(hVolumeAppWnd, &dwProcessId);

			hSndVolProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, dwProcessId);
			if (hSndVolProcess) {
				if (WaitForInputIdle(hSndVolProcess, 0) == 0) { // if not initializing
					if (ValidateSndVolWnd()) {
						ScrollSndVol(wParam, lMousePosParam);
						return FALSE; // False because we didn't open it, it was open
					}
					else {
						BOOL bOpened;
						if (OpenScrollSndVolInternal(wParam, lMousePosParam, hVolumeAppWnd, &bOpened)) {
							return bOpened;
						}
					}
				}
			}
		}

		return FALSE;
	}

	wsprintf(szCommandLine, L"SndVol.exe -f %u", (DWORD)lMousePosParam);

	ZeroMemory(&si, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);

	if (!CreateProcess(NULL, szCommandLine, NULL, NULL, FALSE, ABOVE_NORMAL_PRIORITY_CLASS | CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		return FALSE;
	}

	AllowSetForegroundWindow(pi.dwProcessId);
	ResumeThread(pi.hThread);

	CloseHandle(pi.hThread);
	hSndVolProcess = pi.hProcess;

	return TRUE;
}

BOOL IsSndVolOpen() {
	return (ValidateSndVolProcess() && ValidateSndVolWnd());
}

BOOL ScrollSndVol(WPARAM wParam, LPARAM lMousePosParam) {
	GUITHREADINFO guithreadinfo;

	guithreadinfo.cbSize = sizeof(GUITHREADINFO);

	if (!GetGUIThreadInfo(GetWindowThreadProcessId(hSndVolWnd, NULL), &guithreadinfo)) {
		return FALSE;
	}

	PostMessage(guithreadinfo.hwndFocus, WM_MOUSEWHEEL, wParam, lMousePosParam);
	return TRUE;
}

void SetSndVolTimer() {
	nCloseSndVolTimer = SetTimer(NULL, nCloseSndVolTimer, 1, CloseSndVolTimerProc);
	nCloseSndVolTimerCount = 0;
}

void ResetSndVolTimer() {
	if (nCloseSndVolTimer != 0) { SetSndVolTimer(); }
}

void KillSndVolTimer() {
	if (nCloseSndVolTimer != 0) {
		KillTimer(NULL, nCloseSndVolTimer);
		nCloseSndVolTimer = 0;
	}
}

void CleanupSndVol() {
	KillSndVolTimer();

	if (hSndVolProcess) {
		CloseHandle(hSndVolProcess);
		hSndVolProcess = NULL;
		hSndVolWnd = NULL;
	}
}

BOOL OpenScrollSndVolInternal(WPARAM wParam, LPARAM lMousePosParam, HWND hVolumeAppWnd, BOOL* pbOpened) {
	HWND hSndVolDlg = GetSndVolDlg(hVolumeAppWnd);
	if (hSndVolDlg) {
		if (GetWindowTextLength(hSndVolDlg) == 0) { // Volume control
			if (IsSndVolWndInitialized(hSndVolDlg) && MoveSndVolCenterMouse(hSndVolDlg)) {
				SetForegroundWindow(hVolumeAppWnd);
				PostMessage(hVolumeAppWnd, WM_USER + 35, 0, 0);

				*pbOpened = TRUE;
				return TRUE;
			}
		}
		else if (IsWindowVisible(hSndVolDlg)) { // Another dialog, e.g. volume mixer
			SetForegroundWindow(hVolumeAppWnd);
			PostMessage(hVolumeAppWnd, WM_USER + 35, 0, 0);

			*pbOpened = FALSE;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL ValidateSndVolProcess() {
	if (!hSndVolProcess) { return FALSE; }

	if (WaitForSingleObject(hSndVolProcess, 0) != WAIT_TIMEOUT) {
		CloseHandle(hSndVolProcess);
		hSndVolProcess = NULL;
		hSndVolWnd = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL ValidateSndVolWnd() {
	HWND hForegroundWnd;
	DWORD dwProcessId;
	WCHAR szClass[sizeof("#32770") + 1];

	hForegroundWnd = GetForegroundWindow();

	if (hSndVolWnd == hForegroundWnd) { return TRUE; }

	GetWindowThreadProcessId(hForegroundWnd, &dwProcessId);

	if (GetProcessId(hSndVolProcess) == dwProcessId) {
		GetClassName(hForegroundWnd, szClass, sizeof("#32770") + 1);

		if (lstrcmp(szClass, L"#32770") == 0) {
			hSndVolWnd = hForegroundWnd;
			bCloseOnMouseLeave = FALSE;
			return TRUE;
		}
	}
	hSndVolWnd = NULL;
	return FALSE;
}

void CALLBACK CloseSndVolTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	POINT pt;
	RECT rc;

	if (g_Config.indicator_type == INDICATOR_TYPE::TOOLTIP) {
		nCloseSndVolTimerCount++;
		if (nCloseSndVolTimerCount < g_Config.display_time) { return; }
		HideSndVolTooltip();
	} else if (g_Config.indicator_type == INDICATOR_TYPE::MODERN && CanUseModernIndicator()) {
		if (!bSndVolModernAppeared) {
			if (GetOpenSndVolModernIndicatorWnd()) {
				bSndVolModernAppeared = TRUE;
				nCloseSndVolTimerCount = 1;
				return;
			}
			else {
				nCloseSndVolTimerCount++;
				if (nCloseSndVolTimerCount < g_Config.display_time) { return; }
			}
		} else if (GetOpenSndVolModernIndicatorWnd()) {
			if (!IsCursorUnderSndVolModernIndicatorWnd()) {
				nCloseSndVolTimerCount++;
			} else {
				nCloseSndVolTimerCount = 0;
			}
			if (nCloseSndVolTimerCount < g_Config.display_time) { return; }
			HideSndVolModernIndicator();
		}
		EndSndVolModernIndicatorSession();
	} else {
		if (ValidateSndVolProcess()) {
			if (WaitForInputIdle(hSndVolProcess, 0) != 0) { return; }

			if (ValidateSndVolWnd()) {
				nCloseSndVolTimerCount++;
				if (nCloseSndVolTimerCount < g_Config.display_time) { return; }

				GetCursorPos(&pt);
				GetWindowRect(hSndVolWnd, &rc);

				if (!PtInRect(&rc, pt)) {
					PostMessage(hSndVolWnd, WM_ACTIVATE, MAKEWPARAM(WA_INACTIVE, FALSE), (LPARAM)NULL);
				} else {
					bCloseOnMouseLeave = TRUE;
				}
			}
		}
	}

	KillTimer(NULL, nCloseSndVolTimer);
	nCloseSndVolTimer = 0;
}

HWND GetSndVolDlg(HWND hVolumeAppWnd) {
	HWND hWnd = NULL;
	EnumThreadWindows(GetWindowThreadProcessId(hVolumeAppWnd, NULL), EnumThreadFindSndVolWnd, (LPARAM)&hWnd);
	return hWnd;
}

BOOL CALLBACK EnumThreadFindSndVolWnd(HWND hWnd, LPARAM lParam) {
	WCHAR szClass[16];

	GetClassName(hWnd, szClass, _countof(szClass));
	if (lstrcmp(szClass, L"#32770") == 0) {
		*(HWND*)lParam = hWnd;
		return FALSE;
	}
	return TRUE;
}

BOOL IsSndVolWndInitialized(HWND hWnd) {
	HWND hChildDlg;

	hChildDlg = FindWindowEx(hWnd, NULL, L"#32770", NULL);
	if (!hChildDlg) { return FALSE; }

	if (!(GetWindowLong(hChildDlg, GWL_STYLE) & WS_VISIBLE)) {
		return FALSE;
	}
	return TRUE;
}

BOOL MoveSndVolCenterMouse(HWND hWnd) {
	NOTIFYICONIDENTIFIER notifyiconidentifier;
	BOOL bCompositionEnabled;
	POINT pt;
	SIZE size;
	RECT rc, rcExclude, rcInflate;
	int nInflate;

	ZeroMemory(&notifyiconidentifier, sizeof(NOTIFYICONIDENTIFIER));
	notifyiconidentifier.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	memcpy(&notifyiconidentifier.guidItem, "\x73\xAE\x20\x78\xE3\x23\x29\x42\x82\xC1\xE4\x1C\xB6\x7D\x5B\x9C", sizeof(GUID));

	if (Shell_NotifyIconGetRect(&notifyiconidentifier, &rcExclude) != S_OK) { return FALSE; }

	GetCursorPos(&pt);
	GetWindowRect(hWnd, &rc);

	nInflate = 0;

	if (DwmIsCompositionEnabled(&bCompositionEnabled) == S_OK && bCompositionEnabled) {
		memcpy(&notifyiconidentifier.guidItem, "\x43\x65\x4B\x96\xAD\xBB\xEE\x44\x84\x8A\x3A\x95\xD8\x59\x51\xEA", sizeof(GUID));

		if (Shell_NotifyIconGetRect(&notifyiconidentifier, &rcInflate) == S_OK) {
			nInflate = rcInflate.bottom - rcInflate.top;
			InflateRect(&rc, nInflate, nInflate);
		}
	}

	size.cx = rc.right - rc.left;
	size.cy = rc.bottom - rc.top;

	if (!CalculatePopupWindowPosition(&pt, &size, TPM_CENTERALIGN | TPM_VCENTERALIGN | TPM_VERTICAL | TPM_WORKAREA, &rcExclude, &rc)) {
		return FALSE;
	}
	SetWindowPos(hWnd, NULL, rc.left + nInflate, rc.top + nInflate, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);

	return TRUE;
}

void OnSndVolMouseMove(POINT pt)
{
	HWND hWnd;

	if (bCloseOnMouseLeave) {
		hWnd = WindowFromPoint(pt);

		if (hWnd == hSndVolWnd || IsChild(hSndVolWnd, hWnd)) {
			bCloseOnMouseLeave = FALSE;
			SetSndVolTimer();
		}
	}
}

void OnSndVolMouseClick() {
	bCloseOnMouseLeave = FALSE;
	KillSndVolTimer();
}

void OnSndVolMouseWheel() {
	ResetSndVolTimer();
}


// --------------------------------------------------------
//  ToolTip indicator
// --------------------------------------------------------
void OnSndVolTooltipTimer() {
	HWND hTrayToolbarWnd;
	int index;
	HWND hTooltipWnd;

	if (!bTooltipTimerOn) { return; }
	bTooltipTimerOn = FALSE;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0) { return; }

	hTooltipWnd = (HWND)SendMessage(hTrayToolbarWnd, TB_GETTOOLTIPS, 0, 0);
	if (hTooltipWnd) { ShowWindow(hTooltipWnd, SW_HIDE); }
}

BOOL ShowSndVolTooltip() {
	HWND hTrayToolbarWnd;
	int index;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0) { return FALSE; }

	SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, -1, 0);
	SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, index, 0);

	// Show tooltip
	bTooltipTimerOn = TRUE;
	SetTimer(hTrayToolbarWnd, 0, 0, NULL);

	return TRUE;
}

BOOL HideSndVolTooltip() {
	HWND hTrayToolbarWnd;
	int index;

	index = GetSndVolTrayIconIndex(&hTrayToolbarWnd);
	if (index < 0) { return FALSE; }

	if (SendMessage(hTrayToolbarWnd, TB_GETHOTITEM, 0, 0) == index) {
		SendMessage(hTrayToolbarWnd, TB_SETHOTITEM2, -1, 0);
	}
	return TRUE;
}

int GetSndVolTrayIconIndex(HWND* phTrayToolbarWnd) {
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
//    Other functions
// --------------------------------------------------------
BOOL GetSndVolTrayIconRect(RECT* prc) {
	NOTIFYICONIDENTIFIER notifyiconidentifier;

	ZeroMemory(&notifyiconidentifier, sizeof(NOTIFYICONIDENTIFIER));
	notifyiconidentifier.cbSize = sizeof(NOTIFYICONIDENTIFIER);
	memcpy(&notifyiconidentifier.guidItem, "\x73\xAE\x20\x78\xE3\x23\x29\x42\x82\xC1\xE4\x1C\xB6\x7D\x5B\x9C", sizeof(GUID));

	return (Shell_NotifyIconGetRect(&notifyiconidentifier, prc) == S_OK);
}
