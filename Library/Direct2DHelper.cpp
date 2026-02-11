/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Direct2DHelper.h"
#include "Logging.h"

using namespace Microsoft::WRL;

namespace Direct2D
{
    ComPtr<ID2D1Factory1> g_pD2DFactory;
    ComPtr<IDWriteFactory1> g_pDWriteFactory;
    ComPtr<IWICImagingFactory> g_pWICFactory;

    bool Initialize()
    {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        {
            Logging::Log(LogLevel::Error, L"Failed to initialize COM (0x%08X)", hr);
            return false;
        }

        hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, g_pD2DFactory.GetAddressOf());
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"Failed to create D2D1Factory (0x%08X)", hr);
            return false;
        }

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1), reinterpret_cast<IUnknown**>(g_pDWriteFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"Failed to create DWriteFactory (0x%08X)", hr);
            return false;
        }

        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(g_pWICFactory.GetAddressOf()));
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"Failed to create WICImagingFactory (0x%08X)", hr);
            return false;
        }

        return true;
    }

    void Cleanup()
    {
        g_pWICFactory.Reset();
        g_pDWriteFactory.Reset();
        g_pD2DFactory.Reset();
        CoUninitialize();
    }

    ID2D1Factory1* GetFactory() { return g_pD2DFactory.Get(); }
    IDWriteFactory1* GetWriteFactory() { return g_pDWriteFactory.Get(); }
    IWICImagingFactory* GetWICFactory() { return g_pWICFactory.Get(); }

    D2D1_COLOR_F ColorToD2D(COLORREF color, float alpha)
    {
        return D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f, GetBValue(color) / 255.0f, alpha);
    }

    bool CreateSolidBrush(ID2D1RenderTarget* context, COLORREF color, float alpha, ID2D1SolidColorBrush** brush)
    {
        if (!context) return false;
        HRESULT hr = context->CreateSolidColorBrush(ColorToD2D(color, alpha), brush);
        return SUCCEEDED(hr);
    }

    bool CreateLinearGradientBrush(ID2D1RenderTarget* context, const D2D1_POINT_2F& start, const D2D1_POINT_2F& end, COLORREF color1, float alpha1, COLORREF color2, float alpha2, ID2D1LinearGradientBrush** brush)
    {
        if (!context) return false;

        D2D1_GRADIENT_STOP stops[2];
        stops[0].color = ColorToD2D(color1, alpha1);
        stops[0].position = 0.0f;
        stops[1].color = ColorToD2D(color2, alpha2);
        stops[1].position = 1.0f;

        ComPtr<ID2D1GradientStopCollection> pStops;
        HRESULT hr = context->CreateGradientStopCollection(stops, 2, pStops.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = context->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(start, end), pStops.Get(), brush);
        return SUCCEEDED(hr);
    }

    bool CreateGradientBrush(ID2D1RenderTarget* context, const D2D1_RECT_F& rect, const GradientInfo& info, ID2D1Brush** brush)
    {
        if (!context || info.type == GRADIENT_NONE || info.stops.empty()) return false;

        std::vector<D2D1_GRADIENT_STOP> stops;
        for (const auto& s : info.stops) {
            D2D1_GRADIENT_STOP stop;
            stop.color = ColorToD2D(s.color, s.alpha / 255.0f);
            stop.position = s.position;
            stops.push_back(stop);
        }

        ComPtr<ID2D1GradientStopCollection> pStops;
        HRESULT hr = context->CreateGradientStopCollection(stops.data(), (UINT32)stops.size(), pStops.GetAddressOf());
        if (FAILED(hr)) return false;

        if (info.type == GRADIENT_LINEAR) {
            D2D1_POINT_2F start = FindEdgePoint(info.angle + 180.0f, rect);
            D2D1_POINT_2F end = FindEdgePoint(info.angle, rect);

            ComPtr<ID2D1LinearGradientBrush> pLinear;
            hr = context->CreateLinearGradientBrush(D2D1::LinearGradientBrushProperties(start, end), pStops.Get(), &pLinear);
            if (SUCCEEDED(hr)) {
                *brush = pLinear.Detach();
                return true;
            }
        }
        else if (info.type == GRADIENT_RADIAL) {
            float cx = rect.left + (rect.right - rect.left) / 2.0f;
            float cy = rect.top + (rect.bottom - rect.top) / 2.0f;
            float rx = (rect.right - rect.left) / 2.0f;
            float ry = (rect.bottom - rect.top) / 2.0f;

            ComPtr<ID2D1RadialGradientBrush> pRadial;
            hr = context->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(D2D1::Point2F(cx, cy), D2D1::Point2F(0, 0), rx, ry),
                pStops.Get(), &pRadial);
            if (SUCCEEDED(hr)) {
                *brush = pRadial.Detach();
                return true;
            }
        }

        return false;
    }

    bool CreateBrushFromGradientOrColor(ID2D1RenderTarget* context, const D2D1_RECT_F& rect, const GradientInfo* gradient, COLORREF color, float alpha, ID2D1Brush** brush)
    {
        if (!context || !brush) return false;

        if (gradient && gradient->type != GRADIENT_NONE && !gradient->stops.empty()) {
            if (CreateGradientBrush(context, rect, *gradient, brush)) return true;
        }

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> sBrush;
        if (CreateSolidBrush(context, color, alpha, sBrush.GetAddressOf())) {
            *brush = sBrush.Detach();
            return true;
        }

        return false;
    }

    bool LoadWICBitmapFromFile(const std::wstring& path, IWICBitmap** wicBitmap)
    {
        if (!g_pWICFactory) return false;

        ComPtr<IWICBitmapDecoder> pDecoder;
        HRESULT hr = g_pWICFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, pDecoder.GetAddressOf());
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapFrameDecode> pFrame;
        hr = pDecoder->GetFrame(0, pFrame.GetAddressOf());
        if (FAILED(hr)) return false;

        ComPtr<IWICFormatConverter> pConverter;
        hr = g_pWICFactory->CreateFormatConverter(pConverter.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = pConverter->Initialize(pFrame.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr)) return false;

        hr = g_pWICFactory->CreateBitmapFromSource(pConverter.Get(), WICBitmapCacheOnLoad, wicBitmap);
        return SUCCEEDED(hr);
    }

    bool LoadBitmapFromFile(ID2D1RenderTarget* context, const std::wstring& path, ID2D1Bitmap** bitmap, IWICBitmap** wicBitmap)
    {
        if (!context || !g_pWICFactory) return false;

        ComPtr<IWICBitmap> pWICBitmap;
        if (!LoadWICBitmapFromFile(path, pWICBitmap.GetAddressOf())) return false;

        HRESULT hr = context->CreateBitmapFromWicBitmap(pWICBitmap.Get(), nullptr, bitmap);
        if (SUCCEEDED(hr) && wicBitmap)
        {
            *wicBitmap = pWICBitmap.Detach();
        }
        return SUCCEEDED(hr);
    }

    D2D1_POINT_2F FindEdgePoint(float angle, const D2D1_RECT_F& r)
    {
        float width = r.right - r.left;
        float height = r.bottom - r.top;
        float base_angle = angle;
        while (base_angle < 0.0f) base_angle += 360.0f;
        base_angle = fmodf(base_angle, 360.0f);

        const float M_PI_F = 3.14159265f;
        const float base_radians = base_angle * (M_PI_F / 180.0f);
        const float rectangle_tangent = atan2f(height, width);
        const int quadrant = (int)fmodf(base_angle / 90.0f, 4.0f) + 1;

        const float axis_angle = [&]() -> float {
            switch (quadrant) {
                default:
                case 1: return base_radians - M_PI_F * 0.0f;
                case 2: return M_PI_F * 1.0f - base_radians;
                case 3: return base_radians - M_PI_F * 1.0f;
                case 4: return M_PI_F * 2.0f - base_radians;
            }
        }();

        const float half_area = sqrtf(powf(width, 2.0f) + powf(height, 2.0f)) / 2.0f;
        const float cos_axis = cosf(fabsf(axis_angle - rectangle_tangent));

        return D2D1::Point2F(
            r.left + (width / 2.0f) + (half_area * cos_axis * cosf(base_radians)),
            r.top + (height / 2.0f) + (half_area * cos_axis * sinf(base_radians))
        );
    }
}
