#pragma once

#include "taskbarvol.hpp"

BOOL OpenScrollSndVolInternal(WPARAM wParam, LPARAM lMousePosParam, HWND hVolumeAppWnd, BOOL* pbOpened);
BOOL ValidateSndVolProcess();
BOOL ValidateSndVolWnd();
void CALLBACK CloseSndVolTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
HWND GetSndVolDlg(HWND hVolumeAppWnd);
BOOL CALLBACK EnumThreadFindSndVolWnd(HWND hWnd, LPARAM lParam);
BOOL IsSndVolWndInitialized(HWND hWnd);
BOOL MoveSndVolCenterMouse(HWND hWnd);

// Mouse hook functions
void OnSndVolMouseLeaveClose();
void OnSndVolMouseClick();
void OnSndVolMouseWheel();

// Tooltip indicator functions
BOOL ShowSndVolTooltip();
BOOL HideSndVolTooltip();
int GetSndVolTrayIconIndex(HWND* phTrayToolbarWnd);

// Modern indicator functions
BOOL CanUseModernIndicator();
BOOL ShowSndVolModernIndicator();
BOOL HideSndVolModernIndicator();
void EndSndVolModernIndicatorSession();
BOOL IsCursorUnderSndVolModernIndicatorWnd();
HWND GetOpenSndVolModernIndicatorWnd();
HWND GetSndVolTrayControlWnd();
BOOL CALLBACK EnumThreadFindSndVolTrayControlWnd(HWND hWnd, LPARAM lParam);
