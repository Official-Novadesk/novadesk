/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSAudio.h"
#include "JSUtils.h"
#include "../Utils.h"
#include "../Logging.h"
#include "../PathUtils.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <mmsystem.h>

#pragma comment(lib, "Winmm.lib")

namespace JSApi {

    // Helper to get the default audio endpoint volume interface
    static IAudioEndpointVolume* GetVolumeInterface() {
        HRESULT hr;
        IMMDeviceEnumerator* pEnumerator = NULL;
        IMMDevice* pDevice = NULL;
        IAudioEndpointVolume* pVolume = NULL;

        hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
        if (FAILED(hr)) return NULL;

        hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &pDevice);
        pEnumerator->Release();
        if (FAILED(hr)) return NULL;

        hr = pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&pVolume);
        pDevice->Release();
        if (FAILED(hr)) return NULL;

        return pVolume;
    }

    duk_ret_t js_system_setVolume(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        int volume = duk_get_int(ctx, 0);
        if (volume < 0) volume = 0;
        if (volume > 100) volume = 100;

        IAudioEndpointVolume* pVolume = GetVolumeInterface();
        if (pVolume) {
            float fVolume = (float)volume / 100.0f;
            pVolume->SetMasterVolumeLevelScalar(fVolume, NULL);
            pVolume->Release();
        }

        return 0;
    }

    duk_ret_t js_system_getVolume(duk_context* ctx) {
        IAudioEndpointVolume* pVolume = GetVolumeInterface();
        if (pVolume) {
            float fVolume = 0;
            pVolume->GetMasterVolumeLevelScalar(&fVolume);
            pVolume->Release();
            duk_push_int(ctx, (int)(fVolume * 100.0f + 0.5f));
            return 1;
        }
        duk_push_int(ctx, 0);
        return 1;
    }

    duk_ret_t js_system_playSound(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        std::wstring path = Utils::ToWString(duk_get_string(ctx, 0));
        bool loop = duk_get_boolean_default(ctx, 1, false);

        std::wstring fullPath = ResolveScriptPath(ctx, path);
        
        DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
        if (loop) flags |= SND_LOOP;

        BOOL success = PlaySoundW(fullPath.c_str(), NULL, flags);
        duk_push_boolean(ctx, success);
        return 1;
    }

    duk_ret_t js_system_stopSound(duk_context* ctx) {
        PlaySoundW(NULL, NULL, 0);
        return 0;
    }

    void BindAudioMethods(duk_context* ctx) {
        // "system" object is expected on stack
        duk_push_c_function(ctx, js_system_setVolume, 1);
        duk_put_prop_string(ctx, -2, "setVolume");

        duk_push_c_function(ctx, js_system_getVolume, 0);
        duk_put_prop_string(ctx, -2, "getVolume");

        duk_push_c_function(ctx, js_system_playSound, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "playSound");

        duk_push_c_function(ctx, js_system_stopSound, 0);
        duk_put_prop_string(ctx, -2, "stopSound");
    }
}
