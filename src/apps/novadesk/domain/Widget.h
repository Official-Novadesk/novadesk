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
#include <unordered_map>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "DesktopManager.h"
#include "../render/Element.h"
#include "../render/TextElement.h"
#include "../render/ImageElement.h"
#include "../render/BitmapElement.h"
#include "../render/RotatorElement.h"
#include "../render/ElementLayoutBox.h"
#include "../render/HistogramElement.h"
#include "../render/ButtonElement.h"
#include "../render/BarElement.h"
#include "../render/LineElement.h"
#include "../render/Tooltip.h"
#include "../render/CursorManager.h"
#include "../render/FlexLayoutEngine.h"

#pragma comment(lib, "comctl32.lib")

#include "quickjs.h"

// Forward declarations
class WidgetLayoutHelper;

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
    // Use FlexLayoutConfig from FlexLayoutEngine
    using LayoutConfig = FlexLayoutConfig;
    struct AnimationTarget
    {
        bool hasX = false;
        bool hasY = false;
        bool hasWidth = false;
        bool hasHeight = false;
        bool hasRotate = false;
        float x = 0.0f;
        float y = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float rotate = 0.0f;
        bool hasFontSize = false;
        bool hasFontWeight = false;
        bool hasLetterSpacing = false;
        bool hasFontColor = false;
        float fontSize = 12.0f;
        float fontWeight = 400.0f;
        float letterSpacing = 0.0f;
        float fontColorR = 0.0f;
        float fontColorG = 0.0f;
        float fontColorB = 0.0f;
        float fontAlpha = 255.0f;

        bool HasTransformProps() const
        {
            return hasX || hasY || hasWidth || hasHeight || hasRotate;
        }

        bool HasTextProps() const
        {
            return hasFontSize || hasFontWeight || hasLetterSpacing || hasFontColor;
        }

        bool HasAnyProps() const
        {
            return HasTransformProps() || HasTextProps();
        }
    };

    struct AnimationKeyframe
    {
        float offset = 0.0f;
        std::wstring easing;
        AnimationTarget values;
    };

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
    void SetSkipCloseEventOnDestroy(bool skip) { m_SkipCloseEventOnDestroy = skip; }

    const WidgetOptions& GetOptions() const { return m_Options; }
    HWND GetWindow() const { return m_hWnd; }
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }
    ID2D1DeviceContext* GetDeviceContext() const { return m_pContext.Get(); }

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
    void AddLayoutBox(const PropertyParser::ShapeOptions& options);

    void SetElementProperties(const std::wstring& id, JSContext* ctx, JSValueConst options);
    void SetGroupProperties(const std::wstring& group, JSContext* ctx, JSValueConst options);
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
    void SetLayoutConfig(const std::wstring &id, const LayoutConfig &config);
    bool TryGetLayoutConfig(const std::wstring &id, LayoutConfig &config) const;
    bool IsLayoutContainer(const std::wstring &id) const;
    void ReflowLayout(const std::wstring &id);
    void StartElementAnimation(const std::wstring &id, const AnimationTarget &to, const AnimationTarget &from, int durationMs, const std::wstring &easing, int iterationCount);
    void StartElementKeyframeAnimation(const std::wstring &id, const std::vector<AnimationKeyframe> &keyframes, int durationMs, const std::wstring &easing, int iterationCount);

    friend class WidgetAnimationHelper;
    friend class WidgetLayoutHelper;

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static bool Register();

    static Widget* GetWidgetFromHWND(HWND hWnd);

    void UpdateLayeredWindowContent();

    bool HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);

    void OnContextMenu();
    bool BuildCombinedShapeGeometry(class PathShape* target, const PropertyParser::ShapeOptions& options);
    void ReleaseCombinedConsumes(class PathShape* target);
    void ApplyParsedPropertiesToElement(Element* element, JSContext* ctx, JSValueConst options);
    void UpdateContainerForElement(Element* element, const std::wstring& newContainerId);
    bool WouldCreateContainerCycle(Element* element, Element* container) const;
    void ApplyLayoutForContainer(Element *container);
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
    std::unordered_map<std::wstring, LayoutConfig> m_LayoutConfigs;
    struct ElementAnimation
    {
        std::wstring id;
        std::wstring easing = L"linear";
        DWORD startTick = 0;
        int durationMs = 250;
        int iterationCount = 1;
        int completedIterations = 0;
        bool useKeyframes = false;
        std::vector<float> keyframeOffsets;
        std::vector<std::wstring> keyframeEasings;
        std::vector<AnimationTarget> resolvedStops;
        AnimationTarget from;
        AnimationTarget to;
    };

    std::vector<ElementAnimation> m_Animations;
    Element* m_MouseOverElement = nullptr;
    Element* m_TooltipElement = nullptr;
    int m_IsBatchUpdating = 0;
    
    // Context Menu
    std::vector<MenuItem> m_ContextMenu;
    bool m_ShowDefaultContextMenuItems = true;
    bool m_ContextMenuDisabled = false;

    // Dragging State
    bool m_IsDragging = false;
    bool m_DragThresholdMet = false;
    POINT m_DragStartCursor = { 0, 0 };
    POINT m_DragStartWindow = { 0, 0 };
    bool m_IsElementDragging = false;
    Element *m_DragElement = nullptr;
    bool m_IsMouseOverWidget = false;
    bool m_IsMinimized = false;
    bool m_SkipCloseEventOnDestroy = false;
    CursorManager m_CursorManager;
    HICON m_ToolbarIconHandle = nullptr;
    bool m_ToolbarIconOwned = false;

    void ApplyToolbarStyle();
    void ApplyToolbarIcon();
    void ApplyToolbarTitle();
    void DestroyToolbarIcon();
    void ReleaseRenderSurface();
    
    // Rendering
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_pContext;
    HDC m_hRenderMemDc = nullptr;
    HBITMAP m_hRenderBitmap = nullptr;
    HBITMAP m_hRenderOldBitmap = nullptr;
    void *m_pRenderBitmapBits = nullptr;
    int m_RenderBitmapW = 0;
    int m_RenderBitmapH = 0;

    static const UINT_PTR TIMER_TOPMOST = 2;
    static const UINT_PTR TIMER_TOOLTIP = 3;
    static const UINT_PTR TIMER_CTRL_OVERRIDE = 4;
    static const UINT_PTR TIMER_ANIMATION = 5;
};

#endif
