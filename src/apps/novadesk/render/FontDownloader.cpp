/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontDownloader.h"
#include "FontManager.h"
#include "../shared/Logging.h"
#include "../shared/System.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../domain/Widget.h"

// woff2 / brotli vendored decode API (sources live in src/third_party/woff2)
#include "woff2/decode.h"
#include "woff2/output.h"

#include <windows.h>
#include <shlobj.h>        // SHGetFolderPathW
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <vector>

namespace FontDownloader
{
    namespace
    {
        // -----------------------------------------------------------------------
        // In-progress tracking to avoid double-downloads (keyed by URL)
        // Tracks all pending element/widget requests for a given URL while downloading.
        // -----------------------------------------------------------------------
        struct PendingRequest
        {
            HWND widgetHwnd;
            std::wstring elementId;
        };
        std::mutex g_InProgressMutex;
        std::unordered_map<std::wstring, std::vector<PendingRequest>> g_InProgress;

        // -----------------------------------------------------------------------
        // WOFF2 detection and conversion
        // -----------------------------------------------------------------------

        bool IsWoff2(const std::string &data)
        {
            // WOFF2 magic: 0x774F4632 ('wOF2')
            return data.size() >= 4 &&
                   static_cast<unsigned char>(data[0]) == 0x77 &&
                   static_cast<unsigned char>(data[1]) == 0x4F &&
                   static_cast<unsigned char>(data[2]) == 0x46 &&
                   static_cast<unsigned char>(data[3]) == 0x32;
        }

        // Convert WOFF2 bytes to TTF bytes using vendored google/woff2 library.
        // Returns true on success, false on failure.
        bool ConvertWoff2ToTtf(const std::string &woff2Data, std::string &ttfOut)
        {
            const uint8_t *data = reinterpret_cast<const uint8_t *>(woff2Data.data());
            size_t len = woff2Data.size();

            // Determine output size
            size_t ttfSize = woff2::ComputeWOFF2FinalSize(data, len);
            if (ttfSize == 0)
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2 ComputeWOFF2FinalSize returned 0");
                return false;
            }

            ttfOut.resize(ttfSize);

            woff2::WOFF2StringOut out(&ttfOut);
            if (!woff2::ConvertWOFF2ToTTF(data, len, &out))
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2 conversion failed");
                ttfOut.clear();
                return false;
            }

            return true;
        }

        // -----------------------------------------------------------------------
        // DispatchFontReady — posted back to the main thread via PostMessage
        // -----------------------------------------------------------------------

        void DispatchFontReadyInternal(void *vPayload)
        {
            std::unique_ptr<FontReadyPayload> payload(static_cast<FontReadyPayload *>(vPayload));
            if (!payload)
                return;

            if (payload->cachedDir.empty())
            {
                Logging::Log(LogLevel::Warn, L"FontDownloader: Font download failed for element '%s'",
                             payload->elementId.c_str());
                return;
            }

            // Find the widget by HWND and update the element
            Widget *widget = Widget::GetWidgetFromHWND(payload->widgetHwnd);
            if (!widget)
            {
                Logging::Log(LogLevel::Warn, L"FontDownloader: Widget no longer exists, discarding font for '%s'",
                             payload->elementId.c_str());
                return;
            }

            Logging::Log(LogLevel::Info, L"FontDownloader: Applying downloaded font to element '%s' from '%s'",
                         payload->elementId.c_str(), payload->cachedDir.c_str());

            widget->SetElementFontPath(payload->elementId, payload->cachedDir);
        }

    } // anonymous namespace

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    std::wstring GetCachedDir(const std::wstring &url)
    {
        if (FontManager::HasMemoryFont(url))
            return url;
        return L"";
    }

    void RequestAsync(const std::wstring &url, HWND widgetHwnd, const std::wstring &elementId)
    {
        // Check cache again inside lock to avoid race conditions
        {
            std::lock_guard<std::mutex> lk(g_InProgressMutex);
            if (FontManager::HasMemoryFont(url))
            {
                // Already downloaded — dispatch immediately
                auto *payload = new FontReadyPayload{widgetHwnd, elementId, url};
                HWND msgWnd = JSEngine::GetMessageWindow();
                if (msgWnd)
                {
                    PostMessageW(msgWnd, JSEngine::WM_NOVADESK_DISPATCH,
                                 reinterpret_cast<WPARAM>(&DispatchFontReady),
                                 reinterpret_cast<LPARAM>(payload));
                }
                else
                {
                    delete payload;
                }
                return;
            }

            // Check if already in-progress
            auto it = g_InProgress.find(url);
            if (it != g_InProgress.end())
            {
                Logging::Log(LogLevel::Debug, L"FontDownloader: '%s' is already downloading; queueing request for element '%s'", url.c_str(), elementId.c_str());
                it->second.push_back(PendingRequest{widgetHwnd, elementId});
                return;
            }

            // Add the first request and start the download
            g_InProgress[url].push_back(PendingRequest{widgetHwnd, elementId});
        }

        Logging::Log(LogLevel::Info, L"FontDownloader: Starting async download of '%s'", url.c_str());

        // Capture everything needed by value
        std::thread([url]()
        {
            std::wstring cachedDir;

            std::string rawData;
            bool ok = novadesk::shared::system::WebFetch(url, rawData);

            if (ok && !rawData.empty())
            {
                // Convert WOFF2 → TTF if needed
                if (IsWoff2(rawData))
                {
                    Logging::Log(LogLevel::Info, L"FontDownloader: Detected WOFF2, converting to TTF: '%s'", url.c_str());
                    std::string ttfData;
                    if (ConvertWoff2ToTtf(rawData, ttfData))
                    {
                        rawData = std::move(ttfData);
                    }
                    else
                    {
                        Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2→TTF conversion failed for '%s'", url.c_str());
                        ok = false;
                    }
                }

                if (ok)
                {
                    // Register the font data in memory
                    FontManager::AddMemoryFont(url, rawData);
                    cachedDir = url;
                }
            }
            else
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: Download failed for '%s'", url.c_str());
            }

            std::vector<PendingRequest> pending;
            {
                std::lock_guard<std::mutex> lk(g_InProgressMutex);
                auto it = g_InProgress.find(url);
                if (it != g_InProgress.end())
                {
                    pending = std::move(it->second);
                    g_InProgress.erase(it);
                }
            }

            // Post results back to main thread for all pending requests
            HWND msgWnd = JSEngine::GetMessageWindow();
            if (msgWnd)
            {
                for (const auto &req : pending)
                {
                    auto *payload = new FontReadyPayload{req.widgetHwnd, req.elementId, cachedDir};
                    if (!PostMessageW(msgWnd, JSEngine::WM_NOVADESK_DISPATCH,
                                      reinterpret_cast<WPARAM>(&DispatchFontReady),
                                      reinterpret_cast<LPARAM>(payload)))
                    {
                        delete payload;
                    }
                }
            }
        }).detach();
    }

    void DispatchFontReady(void *payload)
    {
        DispatchFontReadyInternal(payload);
    }

} // namespace FontDownloader
