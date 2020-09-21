//---------------------------------------------------------------------------//
//
// MessageDef.cpp
//  TTB Plugin Template (C++11)
//
//---------------------------------------------------------------------------//

#include <windows.h>

//---------------------------------------------------------------------------//
// Message用文字列
//---------------------------------------------------------------------------//

constexpr auto TTB_HSHELL_ACTIVATESHELLWINDOW_MESSAGE = "TTBase HShell Activate ShellWindow";
constexpr auto TTB_HSHELL_GETMINRECT_MESSAGE          = "TTBase HShell GetMinRect";
constexpr auto TTB_HSHELL_LANGUAGE_MESSAGE            = "TTBase HShell Language";
constexpr auto TTB_HSHELL_REDRAW_MESSAGE              = "TTBase HShell Redraw";
constexpr auto TTB_HSHELL_TASKMAN_MESSAGE             = "TTBase HShell TaskMan";
constexpr auto TTB_HSHELL_WINDOWACTIVATED_MESSAGE     = "TTBase HShell WindowActivated";
constexpr auto TTB_HSHELL_WINDOWCREATED_MESSAGE       = "TTBase HShell WindowCreated";
constexpr auto TTB_HSHELL_WINDOWDESTROYED_MESSAGE     = "TTBase HShell WindowDestroyed";
constexpr auto TTB_HMOUSE_ACTION_MESSAGE              = "TTBase HMouse Action";
constexpr auto TTB_ICON_NOTIFY_MESSAGE                = "TTBase ICON NOTIFY";
constexpr auto TTB_LOAD_DATA_FILE_MESSAGE             = "TTBase LOAD DATA FILE";
constexpr auto TTB_SAVE_DATA_FILE_MESSAGE             = "TTBase SAVE DATA FILE";

//---------------------------------------------------------------------------//
// Message定義
//---------------------------------------------------------------------------//

UINT TTB_HSHELL_ACTIVATESHELLWINDOW { 0 };
UINT TTB_HSHELL_GETMINRECT          { 0 };
UINT TTB_HSHELL_LANGUAGE            { 0 };
UINT TTB_HSHELL_REDRAW              { 0 };
UINT TTB_HSHELL_TASKMAN             { 0 };
UINT TTB_HSHELL_WINDOWACTIVATED     { 0 };
UINT TTB_HSHELL_WINDOWCREATED       { 0 };
UINT TTB_HSHELL_WINDOWDESTROYED     { 0 };
UINT TTB_HMOUSE_ACTION              { 0 };
UINT TTB_ICON_NOTIFY                { 0 };
UINT TTB_LOAD_DATA_FILE             { 0 };
UINT TTB_SAVE_DATA_FILE             { 0 };

//---------------------------------------------------------------------------//
// 取得用関数
//---------------------------------------------------------------------------//

void RegisterMessages(void)
{
    TTB_HSHELL_ACTIVATESHELLWINDOW = ::RegisterWindowMessageA(TTB_HSHELL_ACTIVATESHELLWINDOW_MESSAGE);
    TTB_HSHELL_GETMINRECT          = ::RegisterWindowMessageA(TTB_HSHELL_GETMINRECT_MESSAGE);
    TTB_HSHELL_LANGUAGE            = ::RegisterWindowMessageA(TTB_HSHELL_LANGUAGE_MESSAGE);
    TTB_HSHELL_REDRAW              = ::RegisterWindowMessageA(TTB_HSHELL_REDRAW_MESSAGE);
    TTB_HSHELL_TASKMAN             = ::RegisterWindowMessageA(TTB_HSHELL_TASKMAN_MESSAGE);
    TTB_HSHELL_WINDOWACTIVATED     = ::RegisterWindowMessageA(TTB_HSHELL_WINDOWACTIVATED_MESSAGE);
    TTB_HSHELL_WINDOWCREATED       = ::RegisterWindowMessageA(TTB_HSHELL_WINDOWCREATED_MESSAGE);
    TTB_HSHELL_WINDOWDESTROYED     = ::RegisterWindowMessageA(TTB_HSHELL_WINDOWDESTROYED_MESSAGE);
    TTB_HMOUSE_ACTION              = ::RegisterWindowMessageA(TTB_HMOUSE_ACTION_MESSAGE);
    TTB_ICON_NOTIFY                = ::RegisterWindowMessageA(TTB_ICON_NOTIFY_MESSAGE);
    TTB_LOAD_DATA_FILE             = ::RegisterWindowMessageA(TTB_LOAD_DATA_FILE_MESSAGE);
    TTB_SAVE_DATA_FILE             = ::RegisterWindowMessageA(TTB_SAVE_DATA_FILE_MESSAGE);
}

//---------------------------------------------------------------------------//

// MessageDef.cpp