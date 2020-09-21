#pragma once

//---------------------------------------------------------------------------//
//
// Utility.hpp
//  TTB Plugin Template (C++11)
//
//---------------------------------------------------------------------------//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "Plugin.hpp"

//---------------------------------------------------------------------------//
#if defined(_WIN64) || defined(WIN64)
LPWSTR       CopyString          (LPCWSTR Src);
void         DeleteString        (LPCWSTR Str);
void         GetVersion          (LPWSTR Filename, DWORD* VersionMS, DWORD* VersionLS);
BOOL         ExecutePluginCommand(LPCWSTR pluginName, INT32 CmdID);
#else
LPSTR        CopyString          (LPCWSTR Src);
LPSTR        CopyString          (LPCSTR Src);
void         DeleteString        (LPCSTR Str);
void         GetVersion          (LPSTR Filename, DWORD* VersionMS, DWORD* VersionLS);
BOOL         ExecutePluginCommand(LPCSTR pluginName, INT32 CmdID);
#endif
PLUGIN_INFO* CopyPluginInfo      (const PLUGIN_INFO* Src);
void         FreePluginInfo      (PLUGIN_INFO* PLUGIN_INFO);

//---------------------------------------------------------------------------//

#if NO_WRITELOG
  #define WriteLog(logLevel, format, ...)
#else
    void WriteLog (ERROR_LEVEL logLevel, LPCTSTR format, ...);
#endif

//---------------------------------------------------------------------------//

// Utility.hpp