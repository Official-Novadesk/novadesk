/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontManager.h"
#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include "Utils.h"
#include <filesystem>
#include <algorithm>
#include "PathUtils.h"
#include <dwrite_3.h>
#include <mutex>


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

    // Forward declarations of registry helpers
    std::vector<uint8_t> GetMemoryFont(const std::wstring& url);

    extern Microsoft::WRL::ComPtr<IDWriteInMemoryFontFileLoader> g_pInMemoryLoader;
    extern std::mutex g_MemoryFontsMutex;
    extern std::map<std::wstring, std::vector<uint8_t>> g_MemoryFonts;

    class InMemoryFontFileEnumerator : public IDWriteFontFileEnumerator
    {
    public:
        InMemoryFontFileEnumerator(IDWriteFactory* factory, const std::wstring& url)
            : m_RefCount(1), m_pFactory(factory), m_Url(url), m_CurrentIndex(-1)
        {
            std::vector<uint8_t> buffer = GetMemoryFont(url);
            if (!buffer.empty()) {
                Microsoft::WRL::ComPtr<IDWriteFactory5> factory5;
                HRESULT hr = factory->QueryInterface(IID_PPV_ARGS(&factory5));
                if (SUCCEEDED(hr) && g_pInMemoryLoader) {
                    Microsoft::WRL::ComPtr<IDWriteFontFile> fontFile;
                    hr = g_pInMemoryLoader->CreateInMemoryFontFileReference(
                        factory,
                        buffer.data(),
                        static_cast<UINT32>(buffer.size()),
                        nullptr,
                        &fontFile
                    );
                    if (SUCCEEDED(hr)) {
                        m_FontFile = fontFile;
                        Logging::Log(LogLevel::Info, L"FontManager: Successfully created in-memory font file reference for '%s'", url.c_str());
                    } else {
                        Logging::Log(LogLevel::Error, L"FontManager: CreateInMemoryFontFileReference failed (0x%08X) for '%s'", hr, url.c_str());
                    }
                } else {
                    Logging::Log(LogLevel::Error, L"FontManager: IDWriteFactory5 or g_pInMemoryLoader not available for in-memory font '%s'", url.c_str());
                }
            } else {
                Logging::Log(LogLevel::Error, L"FontManager: InMemory data not found for URL '%s'", url.c_str());
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
            if (m_CurrentIndex == 0 && m_FontFile) {
                *hasCurrentFile = TRUE;
            } else {
                *hasCurrentFile = FALSE;
            }
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile** fontFile) override {
            if (m_CurrentIndex == 0 && m_FontFile) {
                *fontFile = m_FontFile.Get();
                (*fontFile)->AddRef();
                return S_OK;
            }
            *fontFile = nullptr;
            return E_FAIL;
        }

    private:
        ULONG m_RefCount;
        Microsoft::WRL::ComPtr<IDWriteFactory> m_pFactory;
        std::wstring m_Url;
        int m_CurrentIndex;
        Microsoft::WRL::ComPtr<IDWriteFontFile> m_FontFile;
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
            std::wstring keyPath((const wchar_t*)collectionKey, collectionKeySize / sizeof(wchar_t));
            // Remove ANY trailing nulls or junk
            while (!keyPath.empty() && (keyPath.back() == L'\0' || keyPath.back() == L' ' || keyPath.back() == L'\r' || keyPath.back() == L'\n')) {
                keyPath.pop_back();
            }

            if (keyPath.rfind(L"http://", 0) == 0 || keyPath.rfind(L"https://", 0) == 0) {
                Logging::Log(LogLevel::Debug, L"FontManager: Enumerating in-memory file for URL: '%s'", keyPath.c_str());
                *fontFileEnumerator = new InMemoryFontFileEnumerator(factory, keyPath);
            } else {
                Logging::Log(LogLevel::Debug, L"FontManager: Enumerating files for directory key: '%s'", keyPath.c_str());
                *fontFileEnumerator = new DirectoryFontFileEnumerator(factory, keyPath);
            }
            return S_OK;
        }

    private:
        ULONG m_RefCount;
    };

    Microsoft::WRL::ComPtr<IDWriteInMemoryFontFileLoader> g_pInMemoryLoader;
    std::mutex g_MemoryFontsMutex;
    std::map<std::wstring, std::vector<uint8_t>> g_MemoryFonts;

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

        // Try to query IDWriteFactory5 to initialize the in-memory loader
        Microsoft::WRL::ComPtr<IDWriteFactory5> factory5;
        if (SUCCEEDED(Direct2D::GetWriteFactory()->QueryInterface(IID_PPV_ARGS(&factory5)))) {
            hr = factory5->CreateInMemoryFontFileLoader(&g_pInMemoryLoader);
            if (SUCCEEDED(hr)) {
                hr = factory5->RegisterFontFileLoader(g_pInMemoryLoader.Get());
                if (FAILED(hr)) {
                    Logging::Log(LogLevel::Error, L"FontManager: Failed to register memory font file loader (0x%08X)", hr);
                    g_pInMemoryLoader.Reset();
                } else {
                    Logging::Log(LogLevel::Info, L"FontManager: Registered IDWriteInMemoryFontFileLoader successfully");
                }
            } else {
                Logging::Log(LogLevel::Error, L"FontManager: Failed to create memory font file loader (0x%08X)", hr);
            }
        } else {
            Logging::Log(LogLevel::Warn, L"FontManager: IDWriteFactory5 not supported on this Windows version. Online fonts will not function.");
        }

        return true;
    }

    void Cleanup()
    {
        if (g_pInMemoryLoader) {
            Microsoft::WRL::ComPtr<IDWriteFactory5> factory5;
            if (SUCCEEDED(Direct2D::GetWriteFactory()->QueryInterface(IID_PPV_ARGS(&factory5)))) {
                factory5->UnregisterFontFileLoader(g_pInMemoryLoader.Get());
            }
            g_pInMemoryLoader.Reset();
        }

        if (g_pLoader) {
            Direct2D::GetWriteFactory()->UnregisterFontCollectionLoader(g_pLoader.Get());
            g_pLoader = nullptr;
        }
        g_CollectionCache.clear();

        {
            std::lock_guard<std::mutex> lock(g_MemoryFontsMutex);
            g_MemoryFonts.clear();
        }
    }

    Microsoft::WRL::ComPtr<IDWriteFontCollection> GetFontCollection(const std::wstring& directoryPath)
    {
        std::wstring key = directoryPath;
        if (directoryPath.rfind(L"http://", 0) != 0 && directoryPath.rfind(L"https://", 0) != 0) {
            key = PathUtils::ResolvePath(directoryPath, PathUtils::GetExeDir());
        }

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

    // Thread-safe registry implementation
    void AddMemoryFont(const std::wstring& url, const std::string& data)
    {
        std::lock_guard<std::mutex> lock(g_MemoryFontsMutex);
        std::vector<uint8_t> bytes(data.begin(), data.end());
        g_MemoryFonts[url] = std::move(bytes);
        Logging::Log(LogLevel::Info, L"FontManager: Registered memory font for '%s' (%zu bytes)", url.c_str(), g_MemoryFonts[url].size());
    }

    std::vector<uint8_t> GetMemoryFont(const std::wstring& url)
    {
        std::lock_guard<std::mutex> lock(g_MemoryFontsMutex);
        auto it = g_MemoryFonts.find(url);
        if (it != g_MemoryFonts.end()) {
            return it->second;
        }
        return {};
    }

    bool HasMemoryFont(const std::wstring& url)
    {
        std::lock_guard<std::mutex> lock(g_MemoryFontsMutex);
        return g_MemoryFonts.find(url) != g_MemoryFonts.end();
    }
}
