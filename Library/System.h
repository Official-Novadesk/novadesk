/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SYSTEM_H__
#define __NOVADESK_SYSTEM_H__

#include <windows.h>
#include <string>
#include <vector>

enum ZPOSITION
{
	ZPOSITION_ONDESKTOP = -2,
	ZPOSITION_ONBOTTOM = -1,
	ZPOSITION_NORMAL = 0,
	ZPOSITION_ONTOP = 1,
	ZPOSITION_ONTOPMOST = 2
};

struct MonitorInfo
{
	bool active;
	HMONITOR handle;
	RECT screen;
	RECT work;
	std::wstring deviceName;
	std::wstring monitorName;
};

struct MultiMonitorInfo
{
	int vsT, vsL, vsH, vsW;
	int primary;
	int primaryIndex;
	std::vector<MonitorInfo> monitors;
};

class System
{
public:
	static void Initialize(HINSTANCE instance);
	static void Finalize();

	static HWND GetBackmostTopWindow();

	/*
	** Get the helper window handle.
	** The helper window is used for desktop icon management.
	*/
	static HWND GetHelperWindow() { return c_HelperWindow; }

	/*
	** Get the main system window handle.
	*/
	static HWND GetWindow() { return c_Window; }

	static void PrepareHelperWindow(HWND desktopIconsHostWindow = nullptr);
	static HWND GetDesktopIconsHostWindow();

	/*
	** Get the current "Show Desktop" state.
	*/
	static bool GetShowDesktop() { return c_ShowDesktop; }

	/*
	** Set the "Show Desktop" state.
	*/
	static void SetShowDesktop(bool show) { c_ShowDesktop = show; }


	static bool CheckDesktopState(HWND desktopIconsHostWindow);
	static void ChangeZPosInOrder();

	/*
	** Get information about all monitors in the system.
	*/
	static const MultiMonitorInfo& GetMultiMonitorInfo() { return c_Monitors; }

	/*
	** Get the number of monitors in the system.
	*/
	static size_t GetMonitorCount() { return c_Monitors.monitors.size(); }
	
	static bool Execute(const std::wstring& target, const std::wstring& parameters = L"", const std::wstring& workingDir = L"", int show = SW_SHOWNORMAL);


	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
	static HWND GetDefaultShellWindow();
	static bool ShouldUseShellWindowAsDesktopIconsHost();
	
	static HWND c_Window;
	static HWND c_HelperWindow;
	static MultiMonitorInfo c_Monitors;
	static bool c_ShowDesktop;

	static const UINT_PTR TIMER_SHOWDESKTOP = 1;
	static const int INTERVAL_SHOWDESKTOP = 100;
	static const int INTERVAL_RESTOREWINDOWS = 50;
};

#endif
