/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <string>
#include "Element.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dxguid.lib")

namespace Direct2D
{
    bool Initialize();
    void Cleanup();

    ID2D1Factory1* GetFactory();
    IDWriteFactory1* GetWriteFactory();
    IWICImagingFactory* GetWICFactory();

    // Helpers
    bool CreateSolidBrush(ID2D1RenderTarget* context, COLORREF color, float alpha, ID2D1SolidColorBrush** brush);
    bool CreateLinearGradientBrush(ID2D1RenderTarget* context, const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, COLORREF color1, float alpha1, COLORREF color2, float alpha2, ID2D1LinearGradientBrush** brush);
    bool CreateGradientBrush(ID2D1RenderTarget* context, const D2D1_RECT_F& rect, const GradientInfo& info, ID2D1Brush** brush);
    bool CreateBrushFromGradientOrColor(ID2D1RenderTarget* context, const D2D1_RECT_F& rect, const GradientInfo* gradient, COLORREF color, float alpha, ID2D1Brush** brush);
    bool LoadBitmapFromFile(ID2D1RenderTarget* context, const std::wstring& path, ID2D1Bitmap** bitmap, IWICBitmap** wicBitmap = nullptr);
    bool LoadWICBitmapFromFile(const std::wstring& path, IWICBitmap** wicBitmap);
    
    D2D1_COLOR_F ColorToD2D(COLORREF color, float alpha = 1.0f);
    D2D1_POINT_2F FindEdgePoint(float angle, const D2D1_RECT_F& rect);
}
