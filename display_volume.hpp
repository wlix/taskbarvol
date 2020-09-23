#pragma once

#include "taskbarvol.hpp"

#if defined(_WIN64) || defined(WIN64)
#define DEF3264(d32, d64)          (d64)
#else
#define DEF3264(d32, d64)          (d32)
#endif

#define DO2(d7, dx)                          ( (nWinVersion == WIN_VERSION_7) ? (d7) : (dx) )
#define DO2_3264(d7_32, d7_64, dx_32, dx_64) DEF3264(DO2(d7_32, dx_32), DO2(d7_64, dx_64))

#define EV_TASKBAR_OFFSET_FIX(offset)       ((offset) +                                                                                                                                 \
                                            ((nWinVersion == WIN_VERSION_811 && nExplorerQFE >= 17930 && nExplorerQFE < 20000 && (offset) >= 8 * sizeof(LONG_PTR)) ? sizeof(LONG_PTR) : \
                                            ((((nWinVersion == WIN_VERSION_10_R4 && nExplorerQFE >= 677) || (nWinVersion == WIN_VERSION_10_R5 && nExplorerQFE >= 346))                  \
                                              && (offset) >= DEF3264(0x8C, 0xD0)) ? (-(int)sizeof(LONG_PTR)) : 0)))
#define EV_TRAY_UI_OFFSET_FIX(offset)       ((nWinVersion < WIN_VERSION_10_R2) ? EV_TASKBAR_OFFSET_FIX(offset) : (offset))
#define EV_TRAY_UI(lp)                      DO2_3264((lp), (lp), 0, 0 /* omitted from public code */)
#define EV_TASKBAR_TRAY_NOTIFY_WND          ((HWND *)(EV_TRAY_UI(lpTaskbarLongPtr) + EV_TRAY_UI_OFFSET_FIX(           \
                                                     DO2_3264(0xC04, 0xD70, 0, 0 /* omitted from public code */))))
#define EV_TRAY_NOTIFY_TOOLBAR_WND(lp)      ((HWND *)((lp) + DO2_3264(0x29C, 0x308, 0, 0 /* omitted from public code */)))

BOOL OpenScrollSndVol(WPARAM wParam, LPARAM lMousePosParam);
BOOL IsSndVolOpen();
BOOL ScrollSndVol(WPARAM wParam, LPARAM lMousePosParam);
void SetSndVolTimer();
void ResetSndVolTimer();
void KillSndVolTimer();
void CleanupSndVol();
BOOL OpenScrollSndVolInternal(WPARAM wParam, LPARAM lMousePosParam, HWND hVolumeAppWnd, BOOL* pbOpened);
BOOL ValidateSndVolProcess();
BOOL ValidateSndVolWnd();
void CALLBACK CloseSndVolTimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
HWND GetSndVolDlg(HWND hVolumeAppWnd);
BOOL CALLBACK EnumThreadFindSndVolWnd(HWND hWnd, LPARAM lParam);
BOOL IsSndVolWndInitialized(HWND hWnd);
BOOL MoveSndVolCenterMouse(HWND hWnd);

// Mouse hook functions
void OnSndVolMouseMove(POINT pt);
void OnSndVolMouseClick();
void OnSndVolMouseWheel();

// Tooltip indicator functions
void OnSndVolTooltipTimer();
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

// other functions
BOOL GetSndVolTrayIconRect(RECT* prc);

