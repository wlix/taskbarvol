#include "config.hpp"
#include "Utility.hpp"

#include <strsafe.h>
#include <shlwapi.h>

LPCWSTR kMutexName = L"TaskBarVolWin10Mutex";

extern HINSTANCE    g_hInst;
extern CONFIG_DATA  g_Config;

BOOL WritePrivateProfileIntW(LPCWSTR lpAppName, LPCWSTR lpKeyName, int value, LPCWSTR lpFileName) {
	wchar_t szBuff[32];
	wsprintfW(szBuff, L"%d", value);
	return ::WritePrivateProfileStringW(lpAppName, lpKeyName, szBuff, lpFileName);
}

config::config() : m_hMutex(NULL) {
	m_hMutex = ::CreateMutexW(nullptr, FALSE, kMutexName);
	_ASSERT(m_hMutex);
}

config::~config() {
	::CloseHandle(m_hMutex);
}

config& config::get_instance() {
	static config s_instance;
	return s_instance;
}

void config::load_config() {
	mutex_locker lock(m_hMutex);

	wchar_t inipath[MAX_PATH];
	size_t len = ::GetModuleFileNameW(g_hInst, inipath, MAX_PATH);
	::PathRenameExtensionW(inipath, L".ini");

	g_Config.bReverse           = ::GetPrivateProfileIntW(L"Setting", L"Reverse",     0,     inipath);
	g_Config.difference         = ::GetPrivateProfileIntW(L"Setting", L"Difference",  2,     inipath);
	g_Config.display_time       = ::GetPrivateProfileIntW(L"Setting", L"DisplayTime", 2000,  inipath);
	g_Config.display_position.x = ::GetPrivateProfileIntW(L"Setting", L"PositionX",   99,    inipath);
	g_Config.display_position.y = ::GetPrivateProfileIntW(L"Setting", L"PositionY",   96,    inipath);
	g_Config.font_height        = ::GetPrivateProfileIntW(L"Setting", L"FontHeight",  10,    inipath);

	wchar_t szBuf[MAX_PATH];
	::GetPrivateProfileStringW(L"Setting", L"FontName", L"MS UI Gothic", szBuf, sizeof(szBuf) / sizeof(wchar_t), inipath);

	::GetPrivateProfileStringW(L"Setting", L"IndicatorType", L"ToolTip", szBuf, sizeof(szBuf) / sizeof(wchar_t), inipath);
	if (lstrcmpiW(szBuf, L"KeyHack") == 0) {
		g_Config.indicator_type = INDICATOR_TYPE::KEYHACK;
	} else if (lstrcmpiW(szBuf, L"ToolTip") == 0) {
		g_Config.indicator_type = INDICATOR_TYPE::TOOLTIP;
	} else if (lstrcmpiW(szBuf, L"SndVol") == 0) {
		g_Config.indicator_type = INDICATOR_TYPE::SNDVOL;
	} else {
		g_Config.indicator_type = INDICATOR_TYPE::NONE;
	}
}
