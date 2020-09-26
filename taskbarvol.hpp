#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>

enum INDICATOR_TYPE {
	NONE = -1,
	TOOLTIP,
	KEYHACK,
	SNDVOL,
	NUMBER_OF_INDICATOR_TYPE
};

typedef struct _CONFIG_DATA {
	POINT          display_position = { 0, 0 };
	int            display_time = 0;
	int            font_height = 10;
	wchar_t        font_name[MAX_PATH] = L"";
	HFONT          hFont = nullptr;
	BOOL	       bReverse = FALSE;
	int		       difference = 0;
	INDICATOR_TYPE indicator_type = INDICATOR_TYPE::TOOLTIP;
} CONFIG_DATA;
