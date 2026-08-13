/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include "../shared/PathUtils.h"
#include <cmath>
#include <cstring>
#include <vector>
#include <algorithm>
#include <winhttp.h>

using namespace Microsoft::WRL;

namespace Direct2D
{
    ComPtr<ID2D1Factory1> g_pD2DFactory;
    ComPtr<IDWriteFactory1> g_pDWriteFactory;
    ComPtr<IWICImagingFactory> g_pWICFactory;

    namespace
    {
        WICBitmapTransformOptions ExifOrientationToWicTransform(USHORT orientation)
        {
            switch (orientation)
            {
            case 2:  return WICBitmapTransformFlipHorizontal;
            case 3:  return WICBitmapTransformRotate180;
            case 4:  return WICBitmapTransformFlipVertical;
            case 5:  return (WICBitmapTransformOptions)(WICBitmapTransformRotate90 | WICBitmapTransformFlipHorizontal);
            case 6:  return WICBitmapTransformRotate90;
            case 7:  return (WICBitmapTransformOptions)(WICBitmapTransformRotate270 | WICBitmapTransformFlipHorizontal);
            case 8:  return WICBitmapTransformRotate270;
            case 1:
            default: return WICBitmapTransformRotate0;
            }
        }

        bool ReadExifOrientation(IWICBitmapFrameDecode* frame, USHORT& outOrientation)
        {
            if (!frame)
                return false;

            outOrientation = 1;
            ComPtr<IWICMetadataQueryReader> queryReader;
            HRESULT hr = frame->GetMetadataQueryReader(queryReader.GetAddressOf());
            if (FAILED(hr) || !queryReader)
                return false;

            PROPVARIANT value;
            PropVariantInit(&value);
            hr = queryReader->GetMetadataByName(L"/app1/ifd/{ushort=274}", &value);
            if (FAILED(hr))
            {
                PropVariantClear(&value);
                return false;
            }

            bool found = false;
            if (value.vt == VT_UI2)
            {
                outOrientation = value.uiVal;
                found = true;
            }
            else if (value.vt == VT_UI4)
            {
                outOrientation = static_cast<USHORT>(value.ulVal);
                found = true;
            }
            else if ((value.vt == (VT_VECTOR | VT_UI1)) && value.caub.cElems >= sizeof(USHORT))
            {
                USHORT tmp = 1;
                memcpy(&tmp, value.caub.pElems, sizeof(USHORT));
                outOrientation = tmp;
                found = true;
            }

            PropVariantClear(&value);
            return found;
        }
    }

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

    bool LoadWICBitmapFromFile(const std::wstring& path, IWICBitmap** wicBitmap, bool useExifOrientation)
    {
        if (!g_pWICFactory) return false;

        // Check for resource path
        if (path.length() >= 6 && _wcsnicmp(path.c_str(), L"res://", 6) == 0)
        {
            std::wstring resPath = path.substr(6);
            std::wstring resType = L"PNG";
            std::wstring resName;

            size_t slash = resPath.find(L'/');
            if (slash != std::wstring::npos)
            {
                resType = resPath.substr(0, slash);
                resName = resPath.substr(slash + 1);
            }
            else
            {
                resName = resPath;
            }

            // Check if name is numeric
            bool isNumeric = true;
            for (wchar_t c : resName)
            {
                if (!iswdigit(c))
                {
                    isNumeric = false;
                    break;
                }
            }

            LPCWSTR nameParam = resName.c_str();
            if (isNumeric && !resName.empty())
            {
                nameParam = MAKEINTRESOURCEW(_wtoi(resName.c_str()));
            }

            return LoadWICBitmapFromResource(GetModuleHandleW(NULL), nameParam, resType.c_str(), wicBitmap);
        }

        // Check if it's a URL using PathUtils
        if (PathUtils::IsURL(path))
        {
            return LoadWICBitmapFromURL(path, wicBitmap, useExifOrientation);
        }

        ComPtr<IWICBitmapDecoder> pDecoder;
        HRESULT hr = g_pWICFactory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, pDecoder.GetAddressOf());
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapFrameDecode> pFrame;
        hr = pDecoder->GetFrame(0, pFrame.GetAddressOf());
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapSource> pSource = pFrame;
        if (useExifOrientation)
        {
            USHORT orientation = 1;
            if (ReadExifOrientation(pFrame.Get(), orientation))
            {
                const WICBitmapTransformOptions transform = ExifOrientationToWicTransform(orientation);
                if (transform != WICBitmapTransformRotate0)
                {
                    ComPtr<IWICBitmapFlipRotator> pFlipRotator;
                    hr = g_pWICFactory->CreateBitmapFlipRotator(pFlipRotator.GetAddressOf());
                    if (FAILED(hr)) return false;
                    hr = pFlipRotator->Initialize(pSource.Get(), transform);
                    if (FAILED(hr)) return false;
                    pSource = pFlipRotator;
                }
            }
        }

        ComPtr<IWICFormatConverter> pConverter;
        hr = g_pWICFactory->CreateFormatConverter(pConverter.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = pConverter->Initialize(pSource.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeMedianCut);
        if (FAILED(hr)) return false;

        hr = g_pWICFactory->CreateBitmapFromSource(pConverter.Get(), WICBitmapCacheOnLoad, wicBitmap);
        return SUCCEEDED(hr);
    }

    bool DownloadImageFromURL(const std::wstring& url, std::vector<BYTE>& buffer)
    {
        buffer.clear();

        // Parse URL
        URL_COMPONENTS urlComp = { 0 };
        urlComp.dwStructSize = sizeof(urlComp);
        
        WCHAR szHostName[256] = { 0 };
        WCHAR szUrlPath[1024] = { 0 };
        
        urlComp.lpszHostName = szHostName;
        urlComp.dwHostNameLength = sizeof(szHostName) / sizeof(WCHAR);
        urlComp.lpszUrlPath = szUrlPath;
        urlComp.dwUrlPathLength = sizeof(szUrlPath) / sizeof(WCHAR);
        
        if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.length(), 0, &urlComp))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to parse URL: %s", url.c_str());
            return false;
        }

        // Initialize WinHTTP
        HINTERNET hSession = WinHttpOpen(
            L"Novadesk/1.0",
            WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
            
        if (!hSession)
        {
            Logging::Log(LogLevel::Error, L"[novadesk] WinHttpOpen failed");
            return false;
        }

        // Connect to server
        HINTERNET hConnect = WinHttpConnect(
            hSession,
            szHostName,
            urlComp.nPort,
            0);
            
        if (!hConnect)
        {
            WinHttpCloseHandle(hSession);
            Logging::Log(LogLevel::Error, L"[novadesk] WinHttpConnect failed for host: %s", szHostName);
            return false;
        }

        // Open request
        DWORD dwFlags = (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            szUrlPath,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            dwFlags);
            
        if (!hRequest)
        {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Logging::Log(LogLevel::Error, L"[novadesk] WinHttpOpenRequest failed");
            return false;
        }

        // Send request
        BOOL bResults = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0);
            
        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Logging::Log(LogLevel::Error, L"[novadesk] WinHttpSendRequest failed");
            return false;
        }

        // Receive response
        bResults = WinHttpReceiveResponse(hRequest, nullptr);
        if (!bResults)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Logging::Log(LogLevel::Error, L"[novadesk] WinHttpReceiveResponse failed");
            return false;
        }

        // Check status code
        DWORD dwStatusCode = 0;
        DWORD dwSize = sizeof(dwStatusCode);
        WinHttpQueryHeaders(hRequest,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            nullptr,
            &dwStatusCode,
            &dwSize,
            nullptr);
            
        if (dwStatusCode != 200)
        {
            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            Logging::Log(LogLevel::Error, L"[novadesk] HTTP request failed with status code: %d", dwStatusCode);
            return false;
        }

        // Read data
        DWORD dwDownloaded = 0;
        BYTE tempBuffer[4096];
        
        do
        {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
            {
                break;
            }
            
            if (dwSize == 0)
                break;
                
            DWORD dwRead = 0;
            if (!WinHttpReadData(hRequest, tempBuffer, (std::min)((DWORD)sizeof(tempBuffer), dwSize), &dwRead))
            {
                break;
            }
            
            if (dwRead > 0)
            {
                buffer.insert(buffer.end(), tempBuffer, tempBuffer + dwRead);
                dwDownloaded += dwRead;
            }
            
        } while (dwSize > 0);

        // Cleanup
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        // Logging::Log(LogLevel::Info, L"[novadesk] Downloaded %d bytes from URL: %s", dwDownloaded, url.c_str());
        return buffer.size() > 0;
    }

    bool LoadWICBitmapFromURL(const std::wstring& url, IWICBitmap** wicBitmap, bool useExifOrientation)
    {
        if (!g_pWICFactory) return false;

        // Download image data
        std::vector<BYTE> imageData;
        if (!DownloadImageFromURL(url, imageData))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to download image from URL: %s", url.c_str());
            return false;
        }

        // Create stream from memory
        ComPtr<IWICStream> pStream;
        HRESULT hr = g_pWICFactory->CreateStream(pStream.GetAddressOf());
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to create WIC stream");
            return false;
        }

        hr = pStream->InitializeFromMemory(imageData.data(), (DWORD)imageData.size());
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to initialize stream from memory");
            return false;
        }

        // Decode image from stream
        ComPtr<IWICBitmapDecoder> pDecoder;
        hr = g_pWICFactory->CreateDecoderFromStream(
            pStream.Get(),
            nullptr,
            WICDecodeMetadataCacheOnLoad,
            pDecoder.GetAddressOf());
            
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to create decoder from stream");
            return false;
        }

        ComPtr<IWICBitmapFrameDecode> pFrame;
        hr = pDecoder->GetFrame(0, pFrame.GetAddressOf());
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to get frame from decoder");
            return false;
        }

        ComPtr<IWICBitmapSource> pSource = pFrame;
        if (useExifOrientation)
        {
            USHORT orientation = 1;
            if (ReadExifOrientation(pFrame.Get(), orientation))
            {
                const WICBitmapTransformOptions transform = ExifOrientationToWicTransform(orientation);
                if (transform != WICBitmapTransformRotate0)
                {
                    ComPtr<IWICBitmapFlipRotator> pFlipRotator;
                    hr = g_pWICFactory->CreateBitmapFlipRotator(pFlipRotator.GetAddressOf());
                    if (FAILED(hr)) return false;
                    hr = pFlipRotator->Initialize(pSource.Get(), transform);
                    if (FAILED(hr)) return false;
                    pSource = pFlipRotator;
                }
            }
        }

        ComPtr<IWICFormatConverter> pConverter;
        hr = g_pWICFactory->CreateFormatConverter(pConverter.GetAddressOf());
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to create format converter");
            return false;
        }

        hr = pConverter->Initialize(
            pSource.Get(),
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeMedianCut);
            
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to initialize format converter");
            return false;
        }

        hr = g_pWICFactory->CreateBitmapFromSource(pConverter.Get(), WICBitmapCacheOnLoad, wicBitmap);
        if (FAILED(hr))
        {
            Logging::Log(LogLevel::Error, L"[novadesk] Failed to create bitmap from source");
            return false;
        }

        Logging::Log(LogLevel::Info, L"[novadesk] Successfully loaded image from URL: %s", url.c_str());
        return true;
    }

    bool LoadBitmapFromFile(ID2D1RenderTarget* context, const std::wstring& path, ID2D1Bitmap** bitmap, IWICBitmap** wicBitmap, bool useExifOrientation)
    {
        if (!context || !g_pWICFactory) return false;

        ComPtr<IWICBitmap> pWICBitmap;
        if (!LoadWICBitmapFromFile(path, pWICBitmap.GetAddressOf(), useExifOrientation)) return false;

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
        base_angle = std::fmod(base_angle, 360.0f);

        const float M_PI_F = 3.14159265f;
        const float base_radians = base_angle * (M_PI_F / 180.0f);
        const float rectangle_tangent = std::atan2(height, width);
        const int quadrant = (int)std::fmod(base_angle / 90.0f, 4.0f) + 1;

        const float axis_angle = [&]() -> float {
            switch (quadrant) {
                default:
                case 1: return base_radians - M_PI_F * 0.0f;
                case 2: return M_PI_F * 1.0f - base_radians;
                case 3: return base_radians - M_PI_F * 1.0f;
                case 4: return M_PI_F * 2.0f - base_radians;
            }
        }();

        const float half_area = std::sqrt(std::pow(width, 2.0f) + std::pow(height, 2.0f)) / 2.0f;
        const float cos_axis = std::cos(std::fabs(axis_angle - rectangle_tangent));

        return D2D1::Point2F(
            r.left + (width / 2.0f) + (half_area * cos_axis * std::cos(base_radians)),
            r.top + (height / 2.0f) + (half_area * cos_axis * std::sin(base_radians))
        );
    }

    bool LoadWICBitmapFromResource(HMODULE hModule, LPCWSTR resourceName, LPCWSTR resourceType, IWICBitmap** wicBitmap)
    {
        if (!g_pWICFactory) return false;

        HRSRC hResInfo = FindResourceW(hModule, resourceName, resourceType);
        if (!hResInfo)
        {
            Logging::Log(LogLevel::Error, L"[novadesk] FindResourceW failed");
            return false;
        }

        DWORD cbResource = SizeofResource(hModule, hResInfo);
        HGLOBAL hResData = LoadResource(hModule, hResInfo);
        if (!hResData) return false;

        LPVOID pResourceData = LockResource(hResData);
        if (!pResourceData) return false;

        ComPtr<IWICStream> pStream;
        HRESULT hr = g_pWICFactory->CreateStream(pStream.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = pStream->InitializeFromMemory(static_cast<BYTE*>(pResourceData), cbResource);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapDecoder> pDecoder;
        hr = g_pWICFactory->CreateDecoderFromStream(pStream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, pDecoder.GetAddressOf());
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

    bool LoadWICBitmapFromMemory(const BYTE* data, DWORD size, IWICBitmap** wicBitmap)
    {
        if (!g_pWICFactory || !data || size == 0) return false;

        ComPtr<IWICStream> pStream;
        HRESULT hr = g_pWICFactory->CreateStream(pStream.GetAddressOf());
        if (FAILED(hr)) return false;

        hr = pStream->InitializeFromMemory(const_cast<BYTE*>(data), size);
        if (FAILED(hr)) return false;

        ComPtr<IWICBitmapDecoder> pDecoder;
        hr = g_pWICFactory->CreateDecoderFromStream(pStream.Get(), nullptr, WICDecodeMetadataCacheOnLoad, pDecoder.GetAddressOf());
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
}
