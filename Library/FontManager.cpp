/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontManager.h"
#include "Direct2DHelper.h"
#include "Logging.h"
#include "Utils.h"
#include <filesystem>
#include <algorithm>
#include "PathUtils.h"


namespace FontManager
{
    class DirectoryFontFileEnumerator : public IDWriteFontFileEnumerator
    {
    public:
        DirectoryFontFileEnumerator(IDWriteFactory* factory, const std::wstring& directoryPath)
            : m_RefCount(1), m_pFactory(factory), m_DirectoryPath(directoryPath), m_CurrentIndex(-1)
        {
            // Collect all font files in the directory
            try {
                for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
                    if (entry.is_regular_file()) {
                        std::wstring ext = entry.path().extension().wstring();
                        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
                        if (ext == L".ttf" || ext == L".otf" || ext == L".ttc") {
                            Logging::Log(LogLevel::Debug, L"FontManager: Found font file: %s", entry.path().filename().wstring().c_str());
                            m_FilePaths.push_back(entry.path().wstring());
                        }
                    }
                }
                Logging::Log(LogLevel::Info, L"FontManager: Finished scanning '%s'. Found %zu font files.", directoryPath.c_str(), m_FilePaths.size());
            } catch (const std::exception& e) {
                Logging::Log(LogLevel::Error, L"FontManager: Error scanning directory '%s': %S", directoryPath.c_str(), e.what());
            }
        }

        // IUnknown
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
            if (riid == __uuidof(IDWriteFontFileEnumerator) || riid == __uuidof(IUnknown)) {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_RefCount); }
        virtual ULONG STDMETHODCALLTYPE Release() override {
            ULONG res = InterlockedDecrement(&m_RefCount);
            if (res == 0) delete this;
            return res;
        }

        // IDWriteFontFileEnumerator
        virtual HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasCurrentFile) override {
            m_CurrentIndex++;
            if (m_CurrentIndex < (int)m_FilePaths.size()) {
                *hasCurrentFile = TRUE;
                m_pCurrentFile = nullptr;
                return m_pFactory->CreateFontFileReference(m_FilePaths[m_CurrentIndex].c_str(), nullptr, &m_pCurrentFile);
            }
            *hasCurrentFile = FALSE;
            m_pCurrentFile = nullptr;
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile** fontFile) override {
            if (m_pCurrentFile) {
                *fontFile = m_pCurrentFile.Get();
                (*fontFile)->AddRef();
                return S_OK;
            }
            *fontFile = nullptr;
            return E_FAIL;
        }

    private:
        ULONG m_RefCount;
        Microsoft::WRL::ComPtr<IDWriteFactory> m_pFactory;
        std::wstring m_DirectoryPath;
        std::vector<std::wstring> m_FilePaths;
        int m_CurrentIndex;
        Microsoft::WRL::ComPtr<IDWriteFontFile> m_pCurrentFile;
    };

    class DirectoryFontCollectionLoader : public IDWriteFontCollectionLoader
    {
    public:
        DirectoryFontCollectionLoader() : m_RefCount(1) {}

        // IUnknown
        virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override {
            if (riid == __uuidof(IDWriteFontCollectionLoader) || riid == __uuidof(IUnknown)) {
                *ppvObject = this;
                AddRef();
                return S_OK;
            }
            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

        virtual ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_RefCount); }
        virtual ULONG STDMETHODCALLTYPE Release() override {
            ULONG res = InterlockedDecrement(&m_RefCount);
            if (res == 0) delete this;
            return res;
        }

        // IDWriteFontCollectionLoader
        virtual HRESULT STDMETHODCALLTYPE CreateEnumeratorFromKey(
            IDWriteFactory* factory,
            void const* collectionKey,
            UINT32 collectionKeySize,
            IDWriteFontFileEnumerator** fontFileEnumerator) override
        {
        std::wstring directoryPath((const wchar_t*)collectionKey, collectionKeySize / sizeof(wchar_t));
        // Remove ANY trailing nulls or junk
        while (!directoryPath.empty() && (directoryPath.back() == L'\0' || directoryPath.back() == L' ' || directoryPath.back() == L'\r' || directoryPath.back() == L'\n')) {
            directoryPath.pop_back();
        }
        Logging::Log(LogLevel::Debug, L"FontManager: Enumerating files for key: '%s' (Size: %u)", directoryPath.c_str(), collectionKeySize);
        *fontFileEnumerator = new DirectoryFontFileEnumerator(factory, directoryPath);
        return S_OK;
        }

    private:
        ULONG m_RefCount;
    };

    // --- FontManager Implementation ---

    Microsoft::WRL::ComPtr<DirectoryFontCollectionLoader> g_pLoader;
    std::map<std::wstring, Microsoft::WRL::ComPtr<IDWriteFontCollection>> g_CollectionCache;

    bool Initialize()
    {
        g_pLoader = new DirectoryFontCollectionLoader();
        HRESULT hr = Direct2D::GetWriteFactory()->RegisterFontCollectionLoader(g_pLoader.Get());
        if (FAILED(hr)) {
            Logging::Log(LogLevel::Error, L"FontManager: Failed to register font collection loader (0x%08X)", hr);
            return false;
        }
        return true;
    }

    void Cleanup()
    {
        if (g_pLoader) {
            Direct2D::GetWriteFactory()->UnregisterFontCollectionLoader(g_pLoader.Get());
            g_pLoader = nullptr;
        }
        g_CollectionCache.clear();
    }

    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetFontCollection(const std::wstring& directoryPath)
    {
        std::wstring key = PathUtils::ResolvePath(directoryPath, PathUtils::GetExeDir());

        auto it = g_CollectionCache.find(key);
        if (it != g_CollectionCache.end()) {
            return it->second;
        }

        // Create new collection
        Microsoft::WRL::ComPtr<IDWriteFontCollection> pCollection;
        HRESULT hr = Direct2D::GetWriteFactory()->CreateCustomFontCollection(
            g_pLoader.Get(),
            key.c_str(),
            (UINT32)(key.length() * sizeof(wchar_t)),
            &pCollection
        );

        if (SUCCEEDED(hr)) {
            Logging::Log(LogLevel::Info, L"FontManager: Created custom font collection for '%s'", key.c_str());
            g_CollectionCache[key] = pCollection;
            return pCollection;
        } else {
            Logging::Log(LogLevel::Error, L"FontManager: Failed to create font collection for '%s' (0x%08X)", key.c_str(), hr);
            return nullptr;
        }
    }
}
