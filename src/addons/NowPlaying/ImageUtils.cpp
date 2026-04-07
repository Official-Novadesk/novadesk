/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "ImageUtils.h"
#include <fstream>
#include <vector>

using namespace winrt;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::Storage::Streams;
using namespace Gdiplus;

namespace ImageUtils
{
    hstring SaveCover(IRandomAccessStreamReference image)
    {
        try
        {
            auto cover_stream = image.OpenReadAsync().get();
            uint64_t size = cover_stream.Size();
            if (size == 0) return L"";

            auto cover_buffer = Buffer(static_cast<uint32_t>(size));
            
            // Get temp folder path
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            std::filesystem::path dir = std::filesystem::path(tempPath) / L"Novadesk" / L"NowPlaying";
            std::filesystem::create_directories(dir);
            
            auto temp_folder = StorageFolder::GetFolderFromPathAsync(dir.wstring()).get();
            auto cover_file = temp_folder.CreateFileAsync(L"cover.png", CreationCollisionOption::ReplaceExisting).get();

            cover_stream.ReadAsync(cover_buffer, cover_buffer.Capacity(), InputStreamOptions::ReadAhead).get();
            FileIO::WriteBufferAsync(cover_file, cover_buffer).get();

            return cover_file.Path();
        }
        catch (...)
        {
            return L"";
        }
    }

    bool CoverHasTransparentBorder(hstring original)
    {
        const int width = 300;
        const int height = 300;
        const int border = 33;

        Bitmap cover(original.c_str());

        if (cover.GetHeight() != height || cover.GetWidth() != width)
        {
            return false;
        }

        Rect r(0, 0, width, height);
        BitmapData data;

        if (cover.LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &data) != Ok)
        {
            return false;
        }

        BYTE* base = static_cast<BYTE*>(data.Scan0);
        int stride = data.Stride;

        auto isTransparentPixel = [&](int x, int y) -> bool
        {
            BYTE* p = base + y * stride + x * 4;
            // ARGB layout is B,G,R,A
            BYTE a = p[3];
            return a == 0;
        };

        auto checkStrip = [&](int x0, int y0, int x1, int y1) -> bool
        {
            for (int y = y0; y < y1; ++y)
            {
                for (int x = x0; x < x1; ++x)
                {
                    if (!isTransparentPixel(x, y))
                    {
                        return false;
                    }
                }
            }
            return true;
        };

        // Check left border: x = 0 .. 32
        if (!checkStrip(0, 0, border, height))
        {
            cover.UnlockBits(&data);
            return false;
        }

        // Check right border: x = 300-33 .. 299
        if (!checkStrip(width - border, 0, width, height))
        {
            cover.UnlockBits(&data);
            return false;
        }

        cover.UnlockBits(&data);
        return true;
    }

    hstring CropCover(hstring original)
    {
        try
        {
            wchar_t tempPath[MAX_PATH];
            GetTempPathW(MAX_PATH, tempPath);
            std::filesystem::path dir = std::filesystem::path(tempPath) / L"Novadesk" / L"NowPlaying";
            
            auto temp_folder = StorageFolder::GetFolderFromPathAsync(dir.wstring()).get();
            auto cropped_file = temp_folder.CreateFileAsync(L"cover_cropped.png", CreationCollisionOption::ReplaceExisting).get();

            {
                Bitmap b(300, 300);
                Graphics g(&b);
                Bitmap cover(original.c_str());
                Gdiplus::Rect r(0, 0, 300, 300);
                g.DrawImage(&cover, r, 33, 0, 234, 234, UnitPixel);

                const CLSID pngEncoderClsId = { 0x557cf406, 0x1a04, 0x11d3,{ 0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e } };
                b.Save(cropped_file.Path().c_str(), &pngEncoderClsId, NULL);
            }

            return cropped_file.Path();
        }
        catch (...)
        {
            return original;
        }
    }
}
