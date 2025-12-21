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
	std::vector<MonitorInfo> monitors;
};

class System
{
public:
	/*
	** Initialize the System module.
	** Sets up helper windows and initializes multi-monitor information.
	*/
	static void Initialize(HINSTANCE instance);

	/*
	** Finalize the System module.
	** Cleans up resources and destroys helper windows.
	*/
	static void Finalize();

	/*
	** Get the backmost top-level window.
	** Used for z-order management of desktop widgets.
	*/
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

	/*
	** Prepare the helper window for desktop icon management.
	** Can optionally specify a custom desktop icons host window.
	*/
	static void PrepareHelperWindow(HWND desktopIconsHostWindow = nullptr);

	/*
	** Get the window that hosts desktop icons.
	** This is typically the shell window or WorkerW window.
	*/
	static HWND GetDesktopIconsHostWindow();

	/*
	** Get the current "Show Desktop" state.
	*/
	static bool GetShowDesktop() { return c_ShowDesktop; }

	/*
	** Set the "Show Desktop" state.
	*/
	static void SetShowDesktop(bool show) { c_ShowDesktop = show; }

	/*
	** Check if the desktop is currently being shown.
	** Updates internal state based on desktop icons host window visibility.
	*/
	static bool CheckDesktopState(HWND desktopIconsHostWindow);

	/*
	** Change z-order positions for all widgets in the correct order.
	** Ensures widgets maintain their relative z-order positions.
	*/
	static void ChangeZPosInOrder();

	/*
	** Get information about all monitors in the system.
	*/
	static const MultiMonitorInfo& GetMultiMonitorInfo() { return c_Monitors; }

	/*
	** Get the number of monitors in the system.
	*/
	static size_t GetMonitorCount() { return c_Monitors.monitors.size(); }

	/*
	** Window procedure for the system helper window.
	*/
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	/*
	** Callback for enumerating monitors.
	** Used to populate multi-monitor information.
	*/
	static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);

	/*
	** Get the default shell window.
	*/
	static HWND GetDefaultShellWindow();

	/*
	** Determine if the shell window should be used as the desktop icons host.
	*/
	static bool ShouldUseShellWindowAsDesktopIconsHost();
	
	static HWND c_Window;
	static HWND c_HelperWindow;
	static MultiMonitorInfo c_Monitors;
	static bool c_ShowDesktop;

	static const UINT_PTR TIMER_SHOWDESKTOP = 1;
	static const int INTERVAL_SHOWDESKTOP = 250;
	static const int INTERVAL_RESTOREWINDOWS = 100;
};

#endif
