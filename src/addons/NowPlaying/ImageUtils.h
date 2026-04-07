/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include <filesystem>
#include <Windows.h>
#include <gdiplus.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

namespace ImageUtils
{
    winrt::hstring SaveCover(winrt::Windows::Storage::Streams::IRandomAccessStreamReference image);
    bool CoverHasTransparentBorder(winrt::hstring original);
    winrt::hstring CropCover(winrt::hstring original);
}
