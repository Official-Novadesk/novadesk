#pragma once

#include <Windows.h>

/**
 * @file novadesk_addon.h
 * @brief Main header for Novadesk Native Addon SDK.
 * 
 * This header provides an engine-agnostic interface for extending Novadesk with native C++ DLLs.
 * It includes a raw C-compatible Host API and a C++ helper layer for safe registration.
 */

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque context handle representing the JavaScript engine instance.
typedef void* novadesk_context;

/**
 * @struct NovadeskHostAPI
 * @brief Table of function pointers provided by the Novadesk core.
 * 
 * Addons interact with the JavaScript engine exclusively through these functions.
 * Host API functions MUST only be called from the main thread.
 */
struct NovadeskHostAPI {
    /** Export properties to JavaScript */
    void (*RegisterString)(novadesk_context ctx, const char* name, const char* value);
    void (*RegisterNumber)(novadesk_context ctx, const char* name, double value);
    void (*RegisterBool)(novadesk_context ctx, const char* name, int value);
    void (*RegisterObjectStart)(novadesk_context ctx, const char* name);
    void (*RegisterObjectEnd)(novadesk_context ctx, const char* name);
    void (*RegisterArrayString)(novadesk_context ctx, const char* name, const char** values, size_t count);
    void (*RegisterArrayNumber)(novadesk_context ctx, const char* name, const double* values, size_t count);
    void (*RegisterFunction)(novadesk_context ctx, const char* name, int (*func)(novadesk_context ctx), int nargs);
    
    /** Manual Stack Management */
    void (*PushString)(novadesk_context ctx, const char* value);
    void (*PushNumber)(novadesk_context ctx, double value);
    void (*PushBool)(novadesk_context ctx, int value);
    void (*PushNull)(novadesk_context ctx);
    void (*PushObject)(novadesk_context ctx);
    void (*PushArray)(novadesk_context ctx);

    /** Retrieve arguments from JavaScript */
    double (*GetNumber)(novadesk_context ctx, int index);
    const char* (*GetString)(novadesk_context ctx, int index);
    int (*GetBool)(novadesk_context ctx, int index);

    /** Type Validation */
    int (*IsNumber)(novadesk_context ctx, int index);
    int (*IsString)(novadesk_context ctx, int index);
    int (*IsBool)(novadesk_context ctx, int index);
    int (*IsObject)(novadesk_context ctx, int index);
    int (*IsFunction)(novadesk_context ctx, int index);
    int (*IsNull)(novadesk_context ctx, int index);

    /** Object Properties */
    int (*GetProperty)(novadesk_context ctx, int objIndex, const char* name);

    /** Stack & Error Control */
    int (*GetTop)(novadesk_context ctx);
    void (*Pop)(novadesk_context ctx);
    void (*PopN)(novadesk_context ctx, int n);
    void (*ThrowError)(novadesk_context ctx, const char* message);

    /** JavaScript Callbacks */
    void* (*JsGetFunctionPtr)(novadesk_context ctx, int index);
    void (*JsCallFunction)(novadesk_context ctx, void* funcPtr, int nargs);
    void (*JsCallFunctionNoArgs)(novadesk_context ctx, void* funcPtr);
    void (*ArrayPushObject)(novadesk_context ctx);
};

// Function signatures for the DLL entry points
typedef void (*NovadeskAddonInitFn)(novadesk_context ctx, HWND hMsgWnd, const NovadeskHostAPI* host);
typedef void (*NovadeskAddonUnloadFn)();

/**
 * @brief Defines the main entry point for the addon.
 * Called when system.loadAddon() is executed.
 */
#define NOVADESK_ADDON_INIT(ctx, hMsgWnd, host) extern "C" __declspec(dllexport) void NovadeskAddonInit(novadesk_context ctx, HWND hMsgWnd, const NovadeskHostAPI* host)

/**
 * @brief Defines the optional cleanup hook.
 * Called during script reload or application shutdown.
 */
#define NOVADESK_ADDON_UNLOAD() extern "C" __declspec(dllexport) void NovadeskAddonUnload()

#ifdef __cplusplus
}

#include <vector>
#include <string>

namespace novadesk {
    
    /**
     * @class JsFunction
     * @brief C++ wrapper to capture and call a JavaScript function.
     */
    class JsFunction {
    public:
        JsFunction(novadesk_context ctx, const NovadeskHostAPI* host, int idx) : m_ctx(ctx), m_host(host) {
            m_ptr = m_host->JsGetFunctionPtr(m_ctx, idx);
        }

        bool IsValid() const { return m_ptr != nullptr; }

        /// Call the function with no arguments.
        void Call() {
            if (!IsValid()) return;
            m_host->JsCallFunction(m_ctx, m_ptr, 0);
        }

        /// Call the function with one string argument.
        void Call(const char* arg) {
            if (!IsValid()) return;
            m_host->PushString(m_ctx, arg);
            m_host->JsCallFunction(m_ctx, m_ptr, 1);
        }

        /// Call the function with one numeric argument.
        void Call(double arg) {
            if (!IsValid()) return;
            m_host->PushNumber(m_ctx, arg);
            m_host->JsCallFunction(m_ctx, m_ptr, 1);
        }

    private:
        novadesk_context m_ctx;
        const NovadeskHostAPI* m_host;
        void* m_ptr;
    };

    /**
     * @class Dispatcher
     * @brief Mechanism for safe communication from background threads to the main thread.
     */
    class Dispatcher {
    public:
        Dispatcher(HWND hMsgWnd) : m_hWnd(hMsgWnd) {}
        
        /// Schedules a function to be executed on the Novadesk main thread.
        void Dispatch(void (*fn)(void*), void* data = nullptr) {
            if (m_hWnd) PostMessage(m_hWnd, WM_NOVADESK_DISPATCH, (WPARAM)fn, (LPARAM)data);
        }
    private:
        HWND m_hWnd;
        static const UINT WM_NOVADESK_DISPATCH = WM_USER + 101;
    };

    /**
     * @class Addon
     * @brief The main C++ helper for creating addons and registering properties/functions.
     */
    class Addon {
    public:
        /// Creates a primary addon object for the current context.
        Addon(novadesk_context ctx, const NovadeskHostAPI* host) : m_ctx(ctx), m_host(host) {
            m_host->PushObject(m_ctx);
        }

        /// Creates a nested object and populates it using the provided lambda.
        template<typename F>
        void RegisterObject(const char* name, F populateFunc) {
            m_host->RegisterObjectStart(m_ctx, name);
            Addon sub(m_ctx, m_host, true);
            populateFunc(sub);
            m_host->RegisterObjectEnd(m_ctx, name);
        }

        /// Registers a C++ function to be accessible from JavaScript.
        void RegisterFunction(const char* name, int (*func)(novadesk_context ctx), int nargs = 0) {
            m_host->RegisterFunction(m_ctx, name, func, nargs);
        }

        void RegisterString(const char* name, const char* value) { m_host->RegisterString(m_ctx, name, value); }
        void RegisterNumber(const char* name, double value) { m_host->RegisterNumber(m_ctx, name, value); }
        void RegisterBool(const char* name, bool value) { m_host->RegisterBool(m_ctx, name, value ? 1 : 0); }

        /// Registers an array of strings.
        void RegisterArray(const char* name, const std::vector<std::string>& values) {
            std::vector<const char*> ptrs;
            for (const auto& s : values) ptrs.push_back(s.c_str());
            m_host->RegisterArrayString(m_ctx, name, ptrs.data(), (size_t)ptrs.size());
        }

        /// Registers an array of numbers.
        void RegisterArray(const char* name, const std::vector<double>& values) {
            m_host->RegisterArrayNumber(m_ctx, name, values.data(), (size_t)values.size());
        }

        /** Stack & Data Access Utilities */
        int GetTop() { return m_host->GetTop(m_ctx); }
        void Pop() { m_host->Pop(m_ctx); }
        void PopN(int n) { m_host->PopN(m_ctx, n); }
        void ThrowError(const char* msg) { m_host->ThrowError(m_ctx, msg); }

        bool IsNumber(int idx) { return m_host->IsNumber(m_ctx, idx) != 0; }
        bool IsString(int idx) { return m_host->IsString(m_ctx, idx) != 0; }
        bool IsBool(int idx) { return m_host->IsBool(m_ctx, idx) != 0; }
        bool IsObject(int idx) { return m_host->IsObject(m_ctx, idx) != 0; }
        bool IsFunction(int idx) { return m_host->IsFunction(m_ctx, idx) != 0; }
        bool IsNull(int idx) { return m_host->IsNull(m_ctx, idx) != 0; }

        bool GetProperty(int objIndex, const char* name) { return m_host->GetProperty(m_ctx, objIndex, name) != 0; }

        double GetNumber(int idx) { return m_host->GetNumber(m_ctx, idx); }
        const char* GetString(int idx) { return m_host->GetString(m_ctx, idx); }
        bool GetBool(int idx) { return m_host->GetBool(m_ctx, idx) != 0; }

    private:
        Addon(novadesk_context ctx, const NovadeskHostAPI* host, bool) : m_ctx(ctx), m_host(host) {}
        novadesk_context m_ctx;
        const NovadeskHostAPI* m_host;
    };
}
#endif
