#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>

struct FormatItem {
	enum {
#if defined(_WIN64) || defined(WIN64)
		STRING,
#else
		STRING,
#endif
		VOLUME
	} type;
#if defined(_WIN64) || defined(WIN64)
	std::wstring str;
#else
	std::string str;
#endif
};

enum INDICATOR_TYPE {
	TOOLTIP,
	MODERN
};

typedef struct _CONFIG_DATA {
#if defined(_WIN64) || defined(WIN64)
	FormatItem text, title;
#else
	std::string text, title;
#endif
	UINT_PTR       id_timer = 0;
	POINT          display_position = { 0, 0 };
	int            display_time = 0;
	HFONT          hFont = nullptr;
	BOOL           bUseTitle = TRUE;
	HWND           hCallbackWnd = nullptr;
	BOOL           bUseSnc = FALSE;
	BOOL	       bReverse = FALSE;
	int		       difference = 0;
	INDICATOR_TYPE indicator_type;
} CONFIG_DATA;
