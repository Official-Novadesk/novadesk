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
	static void Initialize(HINSTANCE instance);
	static void Finalize();

	static HWND GetBackmostTopWindow();
	static HWND GetHelperWindow() { return c_HelperWindow; }
	static HWND GetWindow() { return c_Window; }
	static void PrepareHelperWindow(HWND desktopIconsHostWindow = nullptr);
	static HWND GetDesktopIconsHostWindow();

	static bool GetShowDesktop() { return c_ShowDesktop; }
	static void SetShowDesktop(bool show) { c_ShowDesktop = show; }

	static bool CheckDesktopState(HWND desktopIconsHostWindow);
	static void ChangeZPosInOrder();

	static const MultiMonitorInfo& GetMultiMonitorInfo() { return c_Monitors; }
	static size_t GetMonitorCount() { return c_Monitors.monitors.size(); }

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
	static const int INTERVAL_SHOWDESKTOP = 500;
	static const int INTERVAL_RESTOREWINDOWS = 100;
};

#endif
