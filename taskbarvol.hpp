#pragma once

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <string>

extern int nWinVersion;
extern WORD nExplorerBuild, nExplorerQFE;
extern LONG_PTR lpTaskbarLongPtr;

#define WIN_VERSION_UNSUPPORTED    (-1)
#define WIN_VERSION_7              0
#define WIN_VERSION_8              1
#define WIN_VERSION_81             2
#define WIN_VERSION_811            3
#define WIN_VERSION_10_T1          4
#define WIN_VERSION_10_T2          5
#define WIN_VERSION_10_R1          6
#define WIN_VERSION_10_R2          7
#define WIN_VERSION_10_R3          8
#define WIN_VERSION_10_R4          9
#define WIN_VERSION_10_R5          10
#define WIN_VERSION_10_19H1        11

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
	MODERN,
	NONE,
	NUMBER_OF_INDICATOR_TYPE
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
