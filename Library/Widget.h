#ifndef __NOVADESK_WIDGET_H__
#define __NOVADESK_WIDGET_H__

#include <windows.h>
#include <string>
#include "System.h"

struct WidgetOptions
{
    int width;
    int height;
    std::wstring backgroundColor;
    ZPOSITION zPos;
    BYTE alpha;
    COLORREF color;
    bool draggable;
    bool clickThrough;
    bool keepOnScreen;
    bool snapEdges;
};

class Widget
{
public:
    Widget(const WidgetOptions& options);
    ~Widget();

    bool Create();
    void Show();
    void ChangeZPos(ZPOSITION zPos, bool all = false);
    
    HWND GetWindow() const { return m_hWnd; }
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static Widget* GetWidgetFromHWND(HWND hWnd);
    
    HWND m_hWnd;
    WidgetOptions m_Options;
    ZPOSITION m_WindowZPosition;
};

#endif
