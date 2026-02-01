/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGET_H__
#define __NOVADESK_WIDGET_H__

#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "System.h"
#include "Element.h"
#include "TextElement.h"
#include "ImageElement.h"
#include "BarElement.h"
#include "Tooltip.h"

#pragma comment(lib, "comctl32.lib")

struct duk_hthread;
typedef struct duk_hthread duk_context;

namespace PropertyParser {
    struct ImageOptions;
    struct TextOptions;
    struct BarOptions;
    struct RoundLineOptions;
}

#include "MenuItem.h"

struct WidgetOptions
{
    std::wstring id;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::wstring backgroundColor = L"rgba(255,255,255,255)";
    ZPOSITION zPos = ZPOSITION_NORMAL;
    BYTE bgAlpha = 255;        // Alpha component of background color (0-255)
    BYTE windowOpacity = 255;  // Overall window opacity (0-255)
    COLORREF color = RGB(255, 255, 255);
    bool draggable = true;
    bool clickThrough = false;
    bool keepOnScreen = false;
    bool snapEdges = true;
    bool m_WDefined = false;
    bool m_HDefined = false;
    bool show = true;
    std::wstring scriptPath;
};

class Widget
{
public:
    Widget(const WidgetOptions& options);

    ~Widget();

    bool Create();

    void Show();
    void Hide();
    void Refresh();
    void SetFocus();
    void UnFocus();

    void BeginUpdate() { m_UpdateCount++; }
    void EndUpdate() { if (--m_UpdateCount == 0) Redraw(); }

    void ChangeZPos(ZPOSITION zPos, bool all = false);
    void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
    void SetWindowPosition(int x, int y, int w, int h);
    void SetWindowOpacity(BYTE opacity);
    void SetBackgroundColor(const std::wstring& colorStr);
    void SetDraggable(bool enable) { m_Options.draggable = enable; }
    void SetClickThrough(bool enable);
    void SetKeepOnScreen(bool enable) { m_Options.keepOnScreen = enable; }
    void SetSnapEdges(bool enable) { m_Options.snapEdges = enable; }

    const WidgetOptions& GetOptions() const { return m_Options; }
    HWND GetWindow() const { return m_hWnd; }
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }

    void AddImage(const PropertyParser::ImageOptions& options);
    void AddText(const PropertyParser::TextOptions& options);
    void AddBar(const PropertyParser::BarOptions& options);
    void AddRoundLine(const PropertyParser::RoundLineOptions& options);

    void SetElementProperties(const std::wstring& id, duk_context* ctx);
    bool RemoveElements(const std::wstring& id = L"");
    void RemoveElements(const std::vector<std::wstring>& ids);
	// Context Menu
    void SetContextMenu(const std::vector<MenuItem>& menu);
    void ClearContextMenu();
    void SetContextMenuDisabled(bool disabled) { m_ContextMenuDisabled = disabled; }
    void SetShowDefaultContextMenuItems(bool show) { m_ShowDefaultContextMenuItems = show; }

    void Redraw();

    Element* FindElementById(const std::wstring& id);
    static bool IsValid(Widget* pWidget);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static bool Register();

    static Widget* GetWidgetFromHWND(HWND hWnd);

    void UpdateLayeredWindowContent();

    bool HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void OnContextMenu();

    HWND m_hWnd;
    Tooltip m_Tooltip;
    WidgetOptions m_Options;
    ZPOSITION m_WindowZPosition;
    std::vector<Element*> m_Elements;
    int m_UpdateCount = 0;
    
    // Context Menu
    std::vector<MenuItem> m_ContextMenu;
    bool m_ShowDefaultContextMenuItems = true;
    bool m_ContextMenuDisabled = false;

    Element* m_MouseOverElement = nullptr;

    // Dragging State
    bool m_IsDragging = false;
    bool m_DragThresholdMet = false;
    POINT m_DragStartCursor = { 0, 0 };
    POINT m_DragStartWindow = { 0, 0 };
    
    // Rendering
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_pContext;

    static const UINT_PTR TIMER_TOPMOST = 2;
    static const UINT_PTR TIMER_TOOLTIP = 3;
};

#endif
