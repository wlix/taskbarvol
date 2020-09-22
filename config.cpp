#include <strsafe.h>
#include <shlwapi.h>

#include "config.hpp"
#include "Utility.hpp"

#pragma comment(lib, "shlwapi.lib")

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

	g_Config.bReverse     = ::GetPrivateProfileIntW(L"Setting", L"Reverse",     0,     inipath);
	g_Config.difference   = ::GetPrivateProfileIntW(L"Setting", L"Difference",  1,     inipath);
	g_Config.bUseTitle    = ::GetPrivateProfileIntW(L"Setting", L"UseTitle",    TRUE,  inipath);
	g_Config.display_time = ::GetPrivateProfileIntW(L"Setting", L"DisplayTime", 2000,  inipath);
	g_Config.bUseSnc      = ::GetPrivateProfileIntW(L"Setting", L"UseSnc",      FALSE, inipath);

	wchar_t szBuf[32];
	::GetPrivateProfileStringW(L"Setting", L"IndicatorType", L"Modern", szBuf, sizeof(szBuf) / sizeof(wchar_t), inipath);
	if (lstrcmpiW(szBuf, L"Modern") == 0) {
		g_Config.indicator_type = MODERN;
	}
}
