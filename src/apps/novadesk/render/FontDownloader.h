/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <windows.h>

/*
** FontDownloader provides async font downloading from URLs (http/https).
** Supports .ttf, .otf, and .woff2 (auto-converted to TTF) formats.
** Downloaded fonts are registered and loaded entirely in-memory using
** IDWriteInMemoryFontFileLoader (re-downloaded every application launch).
**
** Usage:
**   // If already downloaded in memory, returns url immediately:
**   std::wstring path = FontDownloader::GetCachedDir(url);
**
**   // If not yet downloaded, start async download:
**   FontDownloader::RequestAsync(url, widgetHwnd, elementId);
*/
namespace FontDownloader
{
    /*
    ** Returns `url` if the font is already downloaded and loaded in-memory,
    ** or an empty string if not yet loaded.
    ** Safe to call from any thread.
    */
    std::wstring GetCachedDir(const std::wstring &url);

    /*
    ** Starts an asynchronous font download for `url`.
    ** On completion, registers the in-memory font with FontManager,
    ** then posts WM_NOVADESK_DISPATCH to `widgetHwnd` which causes
    ** the engine to call SetElementFontPath(elementId, url) + Redraw().
    ** If url is already downloading or loaded, this is a no-op.
    */
    void RequestAsync(const std::wstring &url, HWND widgetHwnd, const std::wstring &elementId);

    /*
    ** Dispatch callback — called on the main thread from the JS engine dispatcher.
    ** payload must be a FontReadyPayload* allocated with new.
    */
    struct FontReadyPayload
    {
        HWND widgetHwnd;
        std::wstring elementId;
        std::wstring cachedDir;   // holds the URL on success, empty on failure
    };
    void DispatchFontReady(void *payload);
}
