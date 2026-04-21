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
#include "DesktopManager.h"
#include "../render/Element.h"
#include "../render/TextElement.h"
#include "../render/ImageElement.h"
#include "../render/BitmapElement.h"
#include "../render/RotatorElement.h"
#include "../render/HistogramElement.h"
#include "../render/ButtonElement.h"
#include "../render/BarElement.h"
#include "../render/LineElement.h"
#include "../render/Tooltip.h"
#include "../render/CursorManager.h"

#pragma comment(lib, "comctl32.lib")

struct duk_hthread;
typedef struct duk_hthread duk_context;

namespace PropertyParser {
    struct ImageOptions;
    struct TextOptions;
    struct ButtonOptions;
    struct BitmapOptions;
    struct RotatorOptions;
    struct BarOptions;
    struct LineOptions;
    struct HistogramOptions;
    struct RoundLineOptions;
    struct ShapeOptions;
    struct AreaGraphOptions;
}

#include "MenuItem.h"

struct WidgetOptions
{
    std::wstring id;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    std::wstring backgroundColor = L"rgba(0,0,0,0)";
    ZPOSITION zPos = ZPOSITION_NORMAL;
    BYTE bgAlpha = 0;          // Alpha component of background color (0-255)
    BYTE windowOpacity = 255;  // Overall window opacity (0-255)
    COLORREF color = RGB(255, 255, 255);
    GradientInfo bgGradient;
    bool draggable = true;
    bool clickThrough = false;
    bool keepOnScreen = false;
    bool snapEdges = true;
    bool showInToolbar = false;
    std::wstring toolbarIcon;
    std::wstring toolbarTitle;
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
    void Minimize();
    void UnMinimize();
    std::wstring GetTitle() const;

    void ChangeZPos(ZPOSITION zPos, bool all = false);
    void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
    void SetWindowPosition(int x, int y, int w, int h);
    void SetWindowOpacity(BYTE opacity);
    void SetBackgroundColor(const std::wstring& colorStr);
    void SetDraggable(bool enable);
    void SetClickThrough(bool enable);
    void SetKeepOnScreen(bool enable);
    void SetSnapEdges(bool enable);
    void SetShowInToolbar(bool enable);
    void SetToolbarIcon(const std::wstring& path);
    void SetToolbarTitle(const std::wstring& title);

    const WidgetOptions& GetOptions() const { return m_Options; }
    HWND GetWindow() const { return m_hWnd; }
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }

    void AddImage(const PropertyParser::ImageOptions& options);
    void AddText(const PropertyParser::TextOptions& options);
    void AddButton(const PropertyParser::ButtonOptions& options);
    void AddBitmap(const PropertyParser::BitmapOptions& options);
    void AddRotator(const PropertyParser::RotatorOptions& options);
    void AddBar(const PropertyParser::BarOptions& options);
    void AddLine(const PropertyParser::LineOptions& options);
    void AddHistogram(const PropertyParser::HistogramOptions& options);
    void AddRoundLine(const PropertyParser::RoundLineOptions& options);
    void AddShape(const PropertyParser::ShapeOptions& options);
    void AddAreaGraph(const PropertyParser::AreaGraphOptions& options);

    void SetElementProperties(const std::wstring& id, duk_context* ctx);
    void SetGroupProperties(const std::wstring& group, duk_context* ctx);
    void RemoveElementsByGroup(const std::wstring& group);
    bool RemoveElements(const std::wstring& id = L"");
    void RemoveElements(const std::vector<std::wstring>& ids);
	// Context Menu
    void SetContextMenu(const std::vector<MenuItem>& menu);
    void ClearContextMenu();
    void SetContextMenuDisabled(bool disabled) { m_ContextMenuDisabled = disabled; }
    void SetShowDefaultContextMenuItems(bool show) { m_ShowDefaultContextMenuItems = show; }

    void BeginUpdate();
    void EndUpdate();

    void Redraw();

    Element* FindElementById(const std::wstring& id);
    static std::vector<Widget *> &GetAllWidgets();
    static void ClearAllWidgets();
    static bool IsValid(Widget* pWidget);

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static bool Register();

    static Widget* GetWidgetFromHWND(HWND hWnd);

    void UpdateLayeredWindowContent();

    bool HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void OnContextMenu();
    bool BuildCombinedShapeGeometry(class PathShape* target, const PropertyParser::ShapeOptions& options);
    void ReleaseCombinedConsumes(class PathShape* target);
    void ApplyParsedPropertiesToElement(Element* element, duk_context* ctx);
    void UpdateContainerForElement(Element* element, const std::wstring& newContainerId);
    bool WouldCreateContainerCycle(Element* element, Element* container) const;
    void RenderContainerChildren(Element* container);
    bool HitTestContainerChildren(Element* container, int x, int y, Element*& outElement);
    bool HitTestContainerChildrenDetailed(
        Element* container,
        int x,
        int y,
        UINT message,
        WPARAM wParam,
        Element*& outHitElement,
        Element*& outActionElement,
        Element*& outMouseActionElement,
        Element*& outToolTipElement);

private:
    std::wstring m_Id;
    std::wstring m_Name;
    WidgetOptions m_Options;
    HWND m_hWnd;
    Tooltip m_Tooltip;
    ZPOSITION m_WindowZPosition;
    std::vector<Element*> m_Elements;
    Element* m_MouseOverElement = nullptr;
    Element* m_TooltipElement = nullptr;
    bool m_IsBatchUpdating = false;
    
    // Context Menu
    std::vector<MenuItem> m_ContextMenu;
    bool m_ShowDefaultContextMenuItems = true;
    bool m_ContextMenuDisabled = false;

    // Dragging State
    bool m_IsDragging = false;
    bool m_DragThresholdMet = false;
    POINT m_DragStartCursor = { 0, 0 };
    POINT m_DragStartWindow = { 0, 0 };
    bool m_IsMouseOverWidget = false;
    bool m_IsMinimized = false;
    CursorManager m_CursorManager;
    HICON m_ToolbarIconHandle = nullptr;
    bool m_ToolbarIconOwned = false;

    void ApplyToolbarStyle();
    void ApplyToolbarIcon();
    void ApplyToolbarTitle();
    void DestroyToolbarIcon();
    
    // Rendering
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_pContext;

    static const UINT_PTR TIMER_TOPMOST = 2;
    static const UINT_PTR TIMER_TOOLTIP = 3;
};

#endif
