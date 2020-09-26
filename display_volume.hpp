#pragma once

#include "taskbarvol.hpp"

void CALLBACK TimerProc_Tip(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
BOOL ShowToolTip(const wchar_t* szText);
BOOL ShowSndVol(LPARAM lParam);
void close_sndvol_process();
BOOL ShowVolume(LPMSLLHOOKSTRUCT msll);